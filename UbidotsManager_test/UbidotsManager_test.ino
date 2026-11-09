#include "src/WiFiManager/WiFiManager.h"
#include "src/DataManager/DataManager.h"
#include "src/UbidotsManager/UbidotsManager.h"
#include "secrets.h"

void setup() {

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n================================");
    Serial.println("     UBIDOTS MANAGER TEST");
    Serial.println("================================");

    DataManager::begin();

    DataManager::updateSensors(
        25.5,
        60.2,
        35.8,
        true,
        true
    );

    DataManager::updateEncoder(90);
    DataManager::updateServo(90);

    Serial.println("Datos de prueba cargados");

    WiFiManager::begin(WIFI_SSID, WIFI_PSWD);

    xTaskCreate(
        WiFiManager::task,
        "WiFiTask",
        4096,
        NULL,
        2,
        NULL
    );

    UbidotsManager::begin(UBIDOTS_TOKEN, UBIDOTS_DEVICE_LABEL);

    Serial.println("Sistema de prueba iniciado");
}

void loop() {

    if (WiFiManager::isConnected()) {

        Serial.print("WiFi conectado: ");
        Serial.println(WiFiManager::getIP());

    } else {

        Serial.println("Esperando conexion WiFi...");
    }

    delay(3000);
}
