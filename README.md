# ESP32 DAQ + Ubidots

Sistema de adquisición de datos (DAQ) sobre ESP32 que integra sensores, control local con encoder/servo y publicación/control remoto en Ubidots.

El repositorio contiene el proyecto integrado (`ESP32DAQ_Ubidots`) y los proyectos de prueba independientes usados para validar cada módulo antes de la integración.

## Descripción general

- Lee temperatura y humedad con un **DHT11**.
- Mide distancia con un **HC-SR04**.
- Lee la posición de un **encoder rotativo** (0–180°) y su botón.
- Posiciona un **servo SG90** según la posición del encoder.
- Centraliza los datos en `DataManager` (con mutex para acceso concurrente).
- Publica la telemetría en **Ubidots** con timestamp NTP y `context.status`.
- Permite **comandar el servo desde un slider** del dashboard de Ubidots.
- Permite **resetear encoder/servo desde un switch** del dashboard.

## Hardware

| Módulo | Señal | GPIO ESP32 |
| --- | --- | --- |
| DHT11 | DATA | 4 |
| HC-SR04 | TRIG | 5 |
| HC-SR04 | ECHO | 18 |
| Encoder KY-040 | CLK | 32 |
| Encoder KY-040 | DT | 33 |
| Encoder KY-040 | SW | 25 |
| Servo SG90 | PWM | 26 |

> Alimenta el servo con una fuente externa de 5 V y une el GND de la fuente con el GND del ESP32.

## Arquitectura de tareas

| Tarea | Periodo | Responsabilidad |
| --- | --- | --- |
| `loop()` | 50 ms | Aplica comandos remotos, mueve el servo, actualiza `DataManager` y muestra estado por Serial. |
| `SensorTask` | 2 s | Lee DHT11 y HC-SR04 y actualiza `SensorManager`. |
| `WiFiTask` | 5 s / reintentos | Conexión y reconexión WiFi. |
| `UbidotsTask` | 5 s | Construye el JSON y publica la telemetría en Ubidots. |
| `UbidotsControl` | 2 s | Lee `servo_setpoint` y `servo_reset` desde Ubidots y encola los comandos. |

## Flujo de datos

```
DHT11 / HC-SR04 ──► SensorManager ──┐
Encoder ──────────► EncoderManager ─┤
                                    ├──► DataManager ──► UbidotsManager ──► Ubidots (telemetría)
Servo ◄──────────── ServoManager ◄──┘
Ubidots (slider/switch) ──► UbidotsManager (polling) ──► EncoderManager / ServoManager
```

### Telemetría (ESP32 → Ubidots)

| Variable | Descripción |
| --- | --- |
| `temperature` | Temperatura en °C (DHT11). |
| `humidity` | Humedad relativa en % (DHT11). |
| `distance` | Distancia en cm (HC-SR04). |
| `position` | Posición del encoder (0–180). |
| `servo_angle` | Ángulo real del servo (0–180). |

### Control (Ubidots → ESP32)

| Variable | Widget | Descripción |
| --- | --- | --- |
| `servo_setpoint` | Slider (0–180, paso 1) | Fija el ángulo del servo y la posición del encoder. |
| `servo_reset` | Switch (0/1) | Al valer 1, resetea encoder y servo a 0. La ESP32 vuelve a escribir 0 para apagar el switch. |

## Estructura del repositorio

```
Proyecto-Master/
├── README.md
├── .gitignore
├── ESP32DAQ_Ubidots/          # Proyecto integrado (master)
│   ├── ESP32DAQ_Ubidots.ino
│   ├── secrets.example.h
│   └── src/
│       ├── DataManager/
│       ├── SensorManager/
│       ├── EncoderManager/
│       ├── ServoManager/
│       ├── WiFiManager/
│       └── UbidotsManager/
├── DataManager_test/          # Pruebas por módulo
├── SensorManager_test/
├── EncoderManager_Test/
├── ServoManager_test/
├── WiFiManager_test/
└── UbidotsManager_test/
```

Cada proyecto de prueba es un sketch independiente que valida un módulo de forma aislada.

## Configuración

### 1. Credenciales (`secrets.h`)

Cada sketch que use WiFi/Ubidots incluye un `secrets.h` local. Cópialo desde el ejemplo y completa tus datos:

```bash
cp ESP32DAQ_Ubidots/secrets.example.h ESP32DAQ_Ubidots/secrets.h
```

Contenido esperado:

```c
#define WIFI_SSID             "TU_SSID"
#define WIFI_PSWD             "TU_PASSWORD"
#define UBIDOTS_TOKEN         "TU_TOKEN"
#define UBIDOTS_DEVICE_LABEL  "TU_DEVICE_LABEL"
#define UBIDOTS_SETPOINT_VAR  "servo_setpoint"
#define UBIDOTS_RESET_VAR     "servo_reset"
```

`secrets.h` está en `.gitignore`; nunca se sube al repositorio. Los archivos `secrets.example.h` solo contienen placeholders.

### 2. Librerías de Arduino

- **DHT sensor library** (Adafruit)
- **Adafruit Unified Sensor**
- **ESP32Servo**

Placa recomendada: **ESP32 Dev Module**.

### 3. Ubidots

1. Crea el dispositivo con el label configurado en `UBIDOTS_DEVICE_LABEL`.
2. Crea (o deja que la ESP32 cree al publicar) las variables de telemetría.
3. Crea `servo_setpoint` y `servo_reset`.
4. En el dashboard, edita el **Slider**:
   - Device: tu dispositivo.
   - Variable: `servo_setpoint`.
   - Rango 0–180, paso 1.
5. Edita el **Switch**:
   - Device: tu dispositivo.
   - Variable: `servo_reset`.
   - ON = 1, OFF = 0.

## Compilar y flashear

Con Arduino IDE: abre el `.ino` correspondiente, selecciona **ESP32 Dev Module** y carga.

Con `arduino-cli`:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 ESP32DAQ_Ubidots
arduino-cli board list
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 ESP32DAQ_Ubidots
```

## Flujo de trabajo de desarrollo

1. **Validación por módulo**: cada carpeta `*_test` se compila y prueba por separado (sensor, encoder, servo, WiFi, DataManager y Ubidots).
2. **Integración**: los módulos validados se copian a `ESP32DAQ_Ubidots/src/` sin cambios funcionales.
3. **Orquestación**: `ESP32DAQ_Ubidots.ino` crea las tareas y conecta los módulos mediante `DataManager`.
4. **Configuración**: se completan las credenciales en `secrets.h` y se enlazan los widgets del dashboard a las variables de control.
5. **Prueba de extremo a extremo**: telemetría y control bidireccional desde Ubidots.

## Notas

- El control remoto usa **polling HTTP cada 2 s** para mantener la misma pila HTTP del proyecto y evitar dependencias adicionales. Si se requiere respuesta instantánea, se puede migrar a MQTT.
- `WiFiClientSecure::setInsecure()` se usa para pruebas; en producción conviene validar el certificado del servidor.
- Si un token o contraseña estuvo expuesto en texto plano, rótalo antes de publicar el repositorio.
