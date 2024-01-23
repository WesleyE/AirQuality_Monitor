#include "Tasks/PMS5003.h"

#include "LogMacros.h"
#include "events.h"
#include "helpers.h"

static const char *TAG = "PMS5003";

#define PMS5003_WARMUP_TIME 30  // 30 seconds
#define PMS5003_TIME_BETWEEN_READINGS 90  // 30 seconds

void PMS5003::run(void *data) {
    this->setup();

    PMS::DATA pmsData = {0, 0, 0, 0, 0, 0};

    int64_t lastWakeUp = 0;
    int64_t lastMeasurement = 0;
    bool performingMeasurement = false;

    // Perform a measurement every 2 minutes
    for (;;) {
        vTaskDelay(500 / portTICK_RATE_MS);

        int64_t currentTime = getTimeSinceBootInSeconds();
        // Check if it has been 90 seconds after the last measurement
        if (currentTime > lastMeasurement + PMS5003_TIME_BETWEEN_READINGS && !performingMeasurement) {
            ESP_LOGD(TAG, "Waking up device, waiting 30 seconds to perform measurement.");

            performingMeasurement = true;
            lastWakeUp = currentTime;

            pms.wakeUp();
            vTaskDelay(1000 / portTICK_RATE_MS);
            pms.passiveMode();
            vTaskDelay(200 / portTICK_RATE_MS);
        }

        if (performingMeasurement) {
            // Wait for 30 seconds for the readings to stabilize
            if (currentTime > lastWakeUp + PMS5003_WARMUP_TIME) {
                ESP_LOGD(TAG, "30 seconds done, performing measurement");
                pms.requestRead();
                if (pms.readUntil(pmsData)) {
                    PMS5003Reading reading = {.Atmospheric_PM_1_0 = pmsData.PM_AE_UG_1_0,
                                              .Atmospheric_PM_2_5 = pmsData.PM_AE_UG_2_5,
                                              .Atmospheric_PM_10_0 = pmsData.PM_AE_UG_10_0};
                    newPMS5003Reading.emit(reading);

                    if (shouldLogValues) {
                        ESP_LOGI(TAG, "PM 1.0 (ug/m3): %d\t\tPM 2.5 (ug/m3): %d\t\tPM 10.0 (ug/m3): %d\t\t", pmsData.PM_AE_UG_1_0,
                                 pmsData.PM_AE_UG_2_5, pmsData.PM_AE_UG_10_0);
                    }
                } else {
                    SensorFaillure faillure = {.type = SensorType::PMS5003_SENSOR, .fatal = true, .errorMessage = "No data received"};
                    sensorFaillure.emit(faillure);
                    ESP_LOGE(TAG, "No data");
                }

                vTaskDelay(500 / portTICK_RATE_MS);

                lastMeasurement = currentTime;
                performingMeasurement = false;
                ESP_LOGD(TAG, "Device is going to sleep");
                Serial2.flush();
                pms.sleep();
            }
        }
    }
}

void PMS5003::setup() {
    ESP_LOGI(TAG, "Task PMS5003 started");

    // Check if we should log
    shouldLogValues = getShouldLogValues();

    Serial2.flush();
    vTaskDelay(200 / portTICK_RATE_MS);

    Serial2.begin(9600, SERIAL_8N1, PIN_PMS_TX, PIN_PMS_RX);

    vTaskDelay(1000 / portTICK_RATE_MS);
    Serial2.flush();

    pms = PMS(Serial2);
    pms.wakeUp();
    pms.passiveMode();
    vTaskDelay(1000 / portTICK_RATE_MS);
}