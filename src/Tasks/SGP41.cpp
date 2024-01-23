#include "Tasks/SGP41.h"

#include <NOxGasIndexAlgorithm.h>
#include <VOCGasIndexAlgorithm.h>
#include <Wire.h>

#include "LogMacros.h"
#include "common.h"
#include "events.h"
#include "helpers.h"

#define SGP41_INTERVAL 10  // 10 seconds

static const char *TAG = "SGP41";

void SGP41::run(void *data) {
    ESP_LOGI(TAG, "Task SGP41 started");

    sgp41.begin(Wire);

    checkSerialNumber();
    checkSelfTest();

    // Check if we should log
    shouldLogValues = getShouldLogValues();

    uint16_t error;
    char errorMessage[256];
    uint16_t srawVoc = 0;
    uint16_t srawNox = 0;

    uint16_t defaultCompenstaionRh = 0x8000;          // in ticks as defined by SGP41
    uint16_t defaultCompenstaionT = 0x6666;           // in ticks as defined by SGP41
    uint16_t compensationRh = defaultCompenstaionRh;  // in ticks as defined by SGP41
    uint16_t compensationT = defaultCompenstaionT;    // in ticks as defined by SGP41

    VOCGasIndexAlgorithm vocAlgorithm;
    NOxGasIndexAlgorithm noxAlgorithm;

    newBME280Reading.connect([&](BME280Reading reading) {
        // convert temperature and humidity to ticks as defined by SGP41
        // interface
        // NOTE: in case you read RH and T raw signals check out the
        // ticks specification in the datasheet, as they can be different for
        // different sensors
        compensationT = static_cast<uint16_t>((reading.temperature + 45) * 65535 / 175);
        compensationRh = static_cast<uint16_t>(reading.humidity * 65535 / 100);
        if (shouldLogValues) {
            ESP_LOGI(TAG, "Setting new compensation parameters: T: %f\tH: %f", reading.temperature, reading.humidity);
        }
    });

    int64_t lastReadingTime = 0;

    for (;;) {
        int64_t currentTime = getTimeSinceBootInSeconds();

        // Perform conditioning
        if (conditioning_s > 0) {
            // During NOx conditioning (10s) SRAW NOx will remain 0
            error = sgp41.executeConditioning(compensationRh, compensationT, srawVoc);
            ESP_LOGI(TAG, "Conditioning in progress: %d seconds left", conditioning_s);
            conditioning_s--;

            if (error) {
                errorToString(error, errorMessage, 256);
                ESP_LOGE(TAG, "Error trying to execute executeConditioning: %s", errorMessage);
            }
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        // We need to keep the measurements rolling for this sensor
        error = sgp41.measureRawSignals(compensationRh, compensationT, srawVoc, srawNox);

        int32_t vocIndex = vocAlgorithm.process(srawVoc);
        int32_t noxIndex = noxAlgorithm.process(srawNox);

        // Report Measurement
        if (currentTime > lastReadingTime + SGP41_INTERVAL) {
            if (error) {
                errorToString(error, errorMessage, 256);
                ESP_LOGE(TAG, "Error trying to execute executeSmeasureRawSignal: %s", errorMessage);
            } else {
                SGP41Reading reading = {
                    .rawNOx = srawNox,
                    .rawVoc = srawVoc,
                    .vocIndex = vocIndex,
                    .noxIndex = noxIndex,
                };
                newSGP41Reading.emit(reading);
                if (shouldLogValues) {
                    ESP_LOGI(TAG, "SRAW_VOC: %d\tSRAW_NOx:%d\tVOC Index: %d, NOx Index: %d", srawVoc, srawNox, vocIndex, noxIndex);
                }
            }
            lastReadingTime = currentTime;
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void SGP41::checkSelfTest() {
    uint16_t error;
    char errorMessage[256];

    uint16_t testResult;
    error = sgp41.executeSelfTest(testResult);
    bool hasError = false;
    if (error) {
        errorToString(error, errorMessage, 256);
        ESP_LOGE(TAG, "Error trying to execute executeSelfTest: %s", errorMessage);
        hasError = true;
    } else if (testResult != 0xD400) {
        ESP_LOGE(TAG, "executeSelfTest failed with error: %d", testResult);
        hasError = true;
    }

    if (hasError) {
        ESP_LOGE(TAG, "Not continuing the SGP41 Task.");

        SensorFaillure faillure = {.type = SensorType::SGP41_SENSOR, .fatal = true, .errorMessage = "Self test failed"};
        sensorFaillure.emit(faillure);

        for (;;) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }

    ESP_LOGI(TAG, "Selftest successful");
}

void SGP41::checkSerialNumber() {
    uint16_t error;
    char errorMessage[256];

    uint8_t serialNumberSize = 3;
    uint16_t serialNumber[serialNumberSize];

    error = sgp41.getSerialNumber(serialNumber);

    if (error) {
        errorToString(error, errorMessage, 256);
        ESP_LOGE(TAG, "Error trying to execute getSerialNumber(): %s", errorMessage);
        SensorFaillure faillure = {.type = SensorType::SGP41_SENSOR, .fatal = true, .errorMessage = "Could not get serial number"};
        sensorFaillure.emit(faillure);
        ESP_LOGE(TAG, "Not continuing the SGP41 Task.");
        for (;;) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }

    ESP_LOGI(TAG, "Serial number check successful");
}