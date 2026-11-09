#include "UbidotsManager.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <sys/time.h>

#include "../DataManager/DataManager.h"

namespace UbidotsManager {

    static const char* URL =
        "https://industrial.api.ubidots.com/api/v1.6/devices/";

    static constexpr uint32_t SEND_INTERVAL = 5000;
    static constexpr uint32_t NTP_RETRY_INTERVAL = 60000;
    static constexpr uint32_t HTTP_TIMEOUT = 10000;

    static String _token;
    static String _deviceLabel;

    static bool ntpStarted = false;
    static uint32_t lastNtpRequest = 0;

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

}
