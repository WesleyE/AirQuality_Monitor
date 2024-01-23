#include "Tasks/VEML7700.h"

#include "LogMacros.h"
#include "common.h"
#include "events.h"
#include "helpers.h"

static const char *TAG = "VEML7700";

#define VEML7700_INTERVAL 10  // 10 seconds

void VEML7700::run(void *data) {
    ESP_LOGI(TAG, "Task VEML7700 started");

    setup();

    int64_t lastReadingTime = 0;

    for (;;) {
        int64_t currentTime = getTimeSinceBootInSeconds();

        if (currentTime > lastReadingTime + VEML7700_INTERVAL) {
            VEML7700Reading reading = {.lux = veml.readLux(VEML_LUX_AUTO)};
            newVEML7700Reading.emit(reading);

            if (shouldLogValues) {
                ESP_LOGI(TAG, "raw ALS: %d\tRaw White: %d\tLux: %f", veml.readALS(), veml.readWhite(), reading.lux);
            }

            lastReadingTime = currentTime;
        }
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void VEML7700::setup() {
    // Check if we should log
    shouldLogValues = getShouldLogValues();

    vTaskDelay(5000 / portTICK_RATE_MS);
    if (!veml.begin()) {
        SensorFaillure faillure = {.type = SensorType::VEML7700_SENSOR, .fatal = true, .errorMessage = "Could not find sensor"};
        sensorFaillure.emit(faillure);

        ESP_LOGE(TAG, "Could not find a valid VEML7700 sensor!");
        ESP_LOGE(TAG, "Not continuing the VEML7700 Task.");
        for (;;) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }

    switch (veml.getGain()) {
        case VEML7700_GAIN_1:
            ESP_LOGI(TAG, "Gain: 1");
            break;
        case VEML7700_GAIN_2:
            ESP_LOGI(TAG, "Gain: 2");
            break;
        case VEML7700_GAIN_1_4:
            ESP_LOGI(TAG, "Gain: 1/4");
            break;
        case VEML7700_GAIN_1_8:
            ESP_LOGI(TAG, "Gain: 1/8");
            break;
    }

    switch (veml.getIntegrationTime()) {
        case VEML7700_IT_25MS:
            ESP_LOGI(TAG, "Integration Time (ms): 25");
            break;
        case VEML7700_IT_50MS:
            ESP_LOGI(TAG, "Integration Time (ms): 50");
            break;
        case VEML7700_IT_100MS:
            ESP_LOGI(TAG, "Integration Time (ms): 100");
            break;
        case VEML7700_IT_200MS:
            ESP_LOGI(TAG, "Integration Time (ms): 200");
            break;
        case VEML7700_IT_400MS:
            ESP_LOGI(TAG, "Integration Time (ms): 400");
            break;
        case VEML7700_IT_800MS:
            ESP_LOGI(TAG, "Integration Time (ms): 800");
            break;
    }
}