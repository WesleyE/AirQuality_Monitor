#include "Tasks/BME280.h"

#include "LogMacros.h"
#include "common.h"
#include "events.h"
#include "helpers.h"

static const char *TAG = "BME280";

#define SEALEVELPRESSURE_HPA (1013.25)
#define BME280_INTERVAL 10  // 10 seconds

void BME280::run(void *data) {
    ESP_LOGI(TAG, "Task BME280 started");

    setup();

    int64_t lastReadingTime = 0;

    for (;;) {
        int64_t currentTime = getTimeSinceBootInSeconds();

        if (currentTime > lastReadingTime + BME280_INTERVAL) {
            bme.takeForcedMeasurement();

            BME280Reading reading = {.temperature = bme.readTemperature(), .pressure = bme.readPressure(), .humidity = bme.readHumidity()};
            newBME280Reading.emit(reading);
            lastReadingTime = currentTime;

            if (shouldLogValues) {
                ESP_LOGI(TAG, "Temperature %f°C\tPressure %fhPa\tHumidity %f%", reading.temperature, reading.pressure, reading.humidity);
            }
        }

        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void BME280::setup() {
    // Check if we should log
    shouldLogValues = getShouldLogValues();

    unsigned status;
    status = bme.begin(0x76);

    if (!status) {
        SensorFaillure faillure = {.type = SensorType::BME280_SENSOR, .fatal = true, .errorMessage = "Could not connect"};
        sensorFaillure.emit(faillure);

        ESP_LOGE(TAG, "Could not find a valid BME280 sensor!");
        ESP_LOGE(TAG, "SensorID was: 0x%04X", bme.sensorID());
        ESP_LOGE(TAG, "        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085");
        ESP_LOGE(TAG, "   ID of 0x56-0x58 represents a BMP 280,");
        ESP_LOGE(TAG, "        ID of 0x60 represents a BME 280.");
        ESP_LOGE(TAG, "        ID of 0x61 represents a BME 680.");

        ESP_LOGE(TAG, "Not continuing the BME280 Task.");
        for (;;) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }

    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,  // temperature
                    Adafruit_BME280::SAMPLING_X1,  // pressure
                    Adafruit_BME280::SAMPLING_X1,  // humidity
                    Adafruit_BME280::FILTER_OFF);
}