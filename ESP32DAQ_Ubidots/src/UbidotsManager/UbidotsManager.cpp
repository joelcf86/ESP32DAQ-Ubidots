#include "UbidotsManager.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "../DataManager/DataManager.h"
#include "../../secrets.h"

namespace UbidotsManager {

    static const char* URL =
        "https://industrial.api.ubidots.com/api/v1.6/devices/";

    static constexpr uint32_t SEND_INTERVAL = 5000;
    static constexpr uint32_t CONTROL_INTERVAL = 2000;
    static constexpr uint32_t NTP_RETRY_INTERVAL = 60000;
    static constexpr uint32_t HTTP_TIMEOUT = 10000;

    static String _token;
    static String _deviceLabel;

    static bool ntpStarted = false;
    static uint32_t lastNtpRequest = 0;

    static portMUX_TYPE commandMux = portMUX_INITIALIZER_UNLOCKED;
    static int pendingSetpoint = 0;
    static bool hasPendingSetpoint = false;
    static bool pendingReset = false;

    static int lastRemoteSetpoint = -1;
    static float lastRemoteReset = 0.0f;

    static uint64_t getTimestamp() {

        struct timeval tv;
        gettimeofday(&tv, nullptr);

        if (tv.tv_sec < 1700000000) {
            return 0;
        }

        return ((uint64_t)tv.tv_sec * 1000ULL) +
               (tv.tv_usec / 1000ULL);
    }

    static void addValue(
        String& json,
        const char* variable,
        String value,
        uint64_t timestamp,
        const char* status,
        bool& first
    ) {

        if (!first) {
            json += ",";
        }

        json += "\"";
        json += variable;
        json += "\":{";

        json += "\"value\":";
        json += value;

        json += ",\"timestamp\":";
        json += String((unsigned long long)timestamp);

        json += ",\"context\":{";
        json += "\"status\":\"";
        json += status;
        json += "\"}}";

        first = false;
    }

    static String createPayload() {

        DataManager::SystemData data = DataManager::getData();

        uint64_t timestamp = getTimestamp();

        String json = "{";
        bool first = true;

        if (data.dhtValid) {

            const char* temperatureStatus =
                data.temperature < 18.0 ? "cold" :
                data.temperature >= 30.0 ? "hot" :
                "normal";

            addValue(
                json,
                "temperature",
                String(data.temperature, 2),
                timestamp,
                temperatureStatus,
                first
            );

            const char* humidityStatus =
                data.humidity >= 70.0 ? "High humidity" :
                "normal";

            addValue(
                json,
                "humidity",
                String(data.humidity, 2),
                timestamp,
                humidityStatus,
                first
            );
        }

        if (data.distanceValid) {

            addValue(
                json,
                "distance",
                String(data.distance, 2),
                timestamp,
                "measured",
                first
            );
        }

        addValue(
            json,
            "position",
            String(data.encoderPosition),
            timestamp,
            "measured",
            first
        );

        addValue(
            json,
            "servo_angle",
            String(data.servoAngle),
            timestamp,
            "measured",
            first
        );

        json += "}";

        return json;
    }

    static bool getRemoteValue(const String& endpoint, float& value) {

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;

        if (!http.begin(client, endpoint)) {
            return false;
        }

        http.setTimeout(HTTP_TIMEOUT);
        http.addHeader("X-Auth-Token", _token);

        int httpCode = http.GET();

        bool ok = false;

        if (httpCode == 200) {

            String body = http.getString();
            body.trim();

            if (body.length() > 0 && body[0] != '{') {
                value = body.toFloat();
                ok = true;
            }
        }

        http.end();

        return ok;
    }

    static bool postValue(const char* variable, float value) {

        String endpoint = String(URL) + _deviceLabel;

        String payload = "{\"";
        payload += variable;
        payload += "\":{\"value\":";
        payload += String(value, 2);
        payload += "}}";

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;

        if (!http.begin(client, endpoint)) {
            return false;
        }

        http.setTimeout(HTTP_TIMEOUT);
        http.addHeader("X-Auth-Token", _token);
        http.addHeader("Content-Type", "application/json");

        int httpCode = http.POST(payload);

        bool ok = (httpCode >= 200 && httpCode < 300);

        http.end();

        return ok;
    }

    static void controlTask(void* parameter) {

        (void)parameter;

        String setpointUrl =
            String(URL) + _deviceLabel + "/" +
            UBIDOTS_SETPOINT_VAR + "/lv";

        String resetUrl =
            String(URL) + _deviceLabel + "/" +
            UBIDOTS_RESET_VAR + "/lv";

        for (;;) {

            if (WiFi.status() == WL_CONNECTED) {

                float setpoint;

                if (getRemoteValue(setpointUrl, setpoint)) {

                    int angle = (int)lroundf(setpoint);

                    if (angle < 0) angle = 0;
                    if (angle > 180) angle = 180;

                    if (angle != lastRemoteSetpoint) {

                        portENTER_CRITICAL(&commandMux);
                        pendingSetpoint = angle;
                        hasPendingSetpoint = true;
                        portEXIT_CRITICAL(&commandMux);

                        lastRemoteSetpoint = angle;
                    }
                }

                float reset;

                if (getRemoteValue(resetUrl, reset)) {

                    if (reset >= 0.5f) {

                        if (lastRemoteReset < 0.5f) {
                            portENTER_CRITICAL(&commandMux);
                            pendingReset = true;
                            portEXIT_CRITICAL(&commandMux);

                            postValue(UBIDOTS_SETPOINT_VAR, 0.0f);
                            lastRemoteSetpoint = 0;
                        }

                        postValue(UBIDOTS_RESET_VAR, 0.0f);
                    }

                    lastRemoteReset = reset;
                }
            }

            vTaskDelay(pdMS_TO_TICKS(CONTROL_INTERVAL));
        }
    }

    void begin(const char* token, const char* deviceLabel) {

        if (token == nullptr || deviceLabel == nullptr) {
            Serial.println("ERROR: UbidotsManager requiere token y device label");
            return;
        }

        _token = token;
        _deviceLabel = deviceLabel;

        Serial.println("Iniciando UbidotsManager...");

        BaseType_t result = xTaskCreate(
            task,
            "UbidotsTask",
            16384,
            NULL,
            1,
            NULL
        );

        if (result != pdPASS) {
            Serial.println("ERROR: no se pudo crear la tarea Ubidots");
            return;
        }

        result = xTaskCreate(
            controlTask,
            "UbidotsControl",
            16384,
            NULL,
            1,
            NULL
        );

        if (result != pdPASS) {
            Serial.println("ERROR: no se pudo crear la tarea de control");
        }
    }

    void task(void* parameter) {

        (void)parameter;

        String endpoint = String(URL) + _deviceLabel;

        while (true) {

            if (WiFi.status() == WL_CONNECTED) {

                uint64_t timestamp = getTimestamp();

                if (timestamp == 0) {

                    uint32_t now = millis();

                    if (!ntpStarted ||
                        millis() - lastNtpRequest >= NTP_RETRY_INTERVAL) {

                        Serial.println("Sincronizando reloj NTP...");

                        configTime(
                            0,
                            0,
                            "pool.ntp.org",
                            "time.nist.gov"
                        );

                        ntpStarted = true;
                        lastNtpRequest = now;
                    }

                    Serial.println("Esperando sincronizacion NTP...");

                } else {

                    WiFiClientSecure client;

                    // Solo para pruebas
                    client.setInsecure();

                    HTTPClient http;

                    if (http.begin(client, endpoint)) {

                        http.setTimeout(HTTP_TIMEOUT);

                        http.addHeader("X-Auth-Token", _token);
                        http.addHeader(
                            "Content-Type",
                            "application/json"
                        );

                        String payload = createPayload();

                        Serial.println("\nEnviando datos a Ubidots:");
                        Serial.println(payload);

                        int httpCode = http.POST(payload);

                        if (httpCode > 0) {

                            Serial.printf(
                                "HTTP Status: %d\n",
                                httpCode
                            );

                            Serial.println(http.getString());

                            if (httpCode < 200 || httpCode >= 300) {
                                Serial.println(
                                    "Advertencia: respuesta HTTP no exitosa"
                                );
                            }

                        } else {

                            Serial.printf(
                                "Error HTTP: %s\n",
                                http.errorToString(httpCode).c_str()
                            );
                        }

                        http.end();

                    } else {

                        Serial.println(
                            "Error al iniciar conexion HTTP"
                        );
                    }
                }

            } else {

                Serial.println("WiFi desconectado. POST pendiente.");
            }

            vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL));
        }
    }

    bool takeSetpoint(int& angle) {

        bool available = false;

        portENTER_CRITICAL(&commandMux);

        if (hasPendingSetpoint) {
            angle = pendingSetpoint;
            hasPendingSetpoint = false;
            available = true;
        }

        portEXIT_CRITICAL(&commandMux);

        return available;
    }

    bool consumeReset() {

        bool requested = false;

        portENTER_CRITICAL(&commandMux);

        if (pendingReset) {
            pendingReset = false;
            requested = true;
        }

        portEXIT_CRITICAL(&commandMux);

        return requested;
    }

}
