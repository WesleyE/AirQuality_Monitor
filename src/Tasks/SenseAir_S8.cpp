#include "Tasks/SenseAir_S8.h"

#include "LogMacros.h"
#include "common.h"
#include "events.h"
#include "helpers.h"

// see https://github.com/plerup/espsoftwareserial/blob/main/examples/loopback/loopback.ino?
// https://forum.arduino.cc/t/esp32-uart-hardware-serial-ports-help/1011812/4

#define S8_INTERVAL 10  // 10 seconds

static const char *TAG = "SenseAir_S8";

void SenseAir_S8::run(void *data) {
    setup();

    int64_t lastReadingTime = 0;

    calibrateSenseAirS8.connect([&]() { this->calibrate(); });

    for (;;) {
        int64_t currentTime = getTimeSinceBootInSeconds();

        if (currentTime > lastReadingTime + S8_INTERVAL) {
            SenseAirS8Reading reading = {.co2 = sensor_S8->get_co2()};

            // Filter out errors in reading
            if (reading.co2 > 200) {
                newSenseAirS8Reading.emit(reading);
                if (shouldLogValues) {
                    ESP_LOGI(TAG, "CO2: %d", reading.co2);
                }
            } else {
                SensorFaillure faillure = {.type = SensorType::SENSEAIR_S8_SENSOR, .errorMessage = "CO2 reading out of band"};
                sensorFaillure.emit(faillure);
                ESP_LOGE(TAG, "CO2 reading out of band: %d < 200", reading.co2);
            }

            lastReadingTime = currentTime;
        }

        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void SenseAir_S8::setup() {
    ESP_LOGI(TAG, "Task SenseAir_S8 started");

    // Check if we should log
    shouldLogValues = getShouldLogValues();

    vTaskDelay(1000 / portTICK_RATE_MS);

    bool couldConnect = false;
    for (uint8_t i = 0; i < 5; i++) {
        ESP_LOGI(TAG, "Trying to connect to the SenseAir S8 (%d/5)", i + 1);
        couldConnect = this->tryConnect();
        if (couldConnect) {
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    if (!couldConnect) {
        ESP_LOGE(TAG, "Connection retries exhausted, not continuing the SenseAir S8 Task.");
        for (;;) {
            vTaskDelay(5000 / portTICK_RATE_MS);
        }
    }

    ESP_LOGI(TAG, "S8 Init Done");
}

void SenseAir_S8::calibrate() { sensor_S8->manual_calibration(); }

bool SenseAir_S8::tryConnect() {
    Serial1.flush();
    vTaskDelay(200 / portTICK_RATE_MS);

    Serial1.begin(S8_BAUDRATE, SERIAL_8N1, PIN_S8_TX, PIN_S8_RX);

    vTaskDelay(200 / portTICK_RATE_MS);
    Serial1.flush();
    sensor_S8 = new S8_UART(Serial1);

    vTaskDelay(200 / portTICK_RATE_MS);

    sensor_S8->get_firmware_version(sensor.firm_version);
    int len = strlen(sensor.firm_version);
    if (len == 0) {
        SensorFaillure faillure = {.type = SensorType::SENSEAIR_S8_SENSOR, .fatal = true, .errorMessage = "Could not connect"};
        sensorFaillure.emit(faillure);
        ESP_LOGE(TAG, "Could not find a valid SenseAir S8 sensor!");
        return false;
    }

    ESP_LOGI(TAG, "Valid SenseAir S8 sensor found: %s", sensor.firm_version);
    return true;
}