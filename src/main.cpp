#include "DeviceSetup.h"
#include "LogMacros.h"
#include "Tasks/BME280.h"
#include "Tasks/HTTP.h"
#include "Tasks/Leds.h"
#include "Tasks/Logging.h"
#include "Tasks/MQTT.h"
#include "Tasks/PMS5003.h"
#include "Tasks/SGP41.h"
#include "Tasks/SenseAir_S8.h"
#include "Tasks/SensorRepository.h"
#include "Tasks/VEML7700.h"
#include "common.h"
#include "helpers.h"

BME280 bme280Task;
SGP41 sgp41Task;
VEML7700 veml7700Task;
SenseAir_S8 senseairs8Task;
PMS5003 pms5003Task;
Leds ledsTask;
SensorRepository repository;
MQTT mqtt;
HTTP http(&repository);
Logging logging;

static const char *TAG = "MAIN";

#if (!PLATFORMIO)
    // Enable Arduino-ESP32 logging in Arduino IDE
    #ifdef CORE_DEBUG_LEVEL
        #undef CORE_DEBUG_LEVEL
    #endif
    #ifdef LOG_LOCAL_LEVEL
        #undef LOG_LOCAL_LEVEL
    #endif

    #define CORE_DEBUG_LEVEL 4
    #define LOG_LOCAL_LEVEL CORE_DEBUG_LEVEL
#endif

void setup() {
    vTaskDelay(2500 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "LOG_LOCAL_LEVEL %d", LOG_LOCAL_LEVEL);
    ESP_LOGI(TAG, "AirQuality Startup - Version %s - Serial Number %s", AIRQUALITY_VERSION, getSerialNumber());

    DeviceSetup::setup();

    vTaskDelay(1000 / portTICK_RATE_MS);

    logging.start();

    vTaskDelay(500 / portTICK_RATE_MS);

    ESP_LOGI(TAG, "AirQuality Starting");

    repository.start();
    bme280Task.start();
    veml7700Task.start();
    senseairs8Task.start();
    ledsTask.start();
    pms5003Task.start();
    sgp41Task.start();
    mqtt.start();
    http.isInProvisioningMode = false;
    http.start();
}

void loop() { vTaskDelay(1000 / portTICK_RATE_MS); }