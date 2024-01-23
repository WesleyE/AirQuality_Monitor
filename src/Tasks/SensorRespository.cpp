
#include "LogMacros.h"
#include "Tasks/SensorRepository.h"
#include "common.h"
#include "events.h"
#include "helpers.h"

static const char *TAG = "SensorRepository";

#define SENSOR_REPORT_INTERVAL 60  // 1 minute

void SensorRepository::run(void *data) {
    setup();

    int64_t lastReportTime = 0;

    for (;;) {
        int64_t currentTime = getTimeSinceBootInSeconds();
        if (currentTime > lastReportTime + SENSOR_REPORT_INTERVAL) {
            ESP_LOGI(TAG, "Sending new report");
            sensorReport.emit(this->getLatestReadings());

            // Generate levels
            this->latestLevels = levels.getLevels(this->getLatestReadings());
            sensorLevelsReport.emit(this->latestLevels);
            

            lastReportTime = currentTime;
        }
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void SensorRepository::setup() {
    ESP_LOGI(TAG, "Task SensorRepository started");

    // Connect to all sensor readings
    newVEML7700Reading.connect([&](VEML7700Reading reading) {
        latestReadings.latestVEML7700Reading = reading;
        latestReadings.lastVEML7700ReadingTime = getTimeSinceBootInSeconds();
    });

    newBME280Reading.connect([&](BME280Reading reading) {
        latestReadings.latestBME280Reading = reading;
        latestReadings.lastBME280ReadingTime = getTimeSinceBootInSeconds();
    });

    newSenseAirS8Reading.connect([&](SenseAirS8Reading reading) {
        latestReadings.latestSenseAirS8Reading = reading;
        latestReadings.lastSenseAirS8ReadingTime = getTimeSinceBootInSeconds();
    });

    newSGP41Reading.connect([&](SGP41Reading reading) {
        latestReadings.latestSGP41Reading = reading;
        latestReadings.lastSGP41ReadingTime = getTimeSinceBootInSeconds();
    });

    newPMS5003Reading.connect([&](PMS5003Reading reading) {
        latestReadings.latestPMS5003Reading = reading;
        latestReadings.lastPMS5003ReadingTime = getTimeSinceBootInSeconds();
    });

    // Connect to sensor faillures
    sensorFaillure.connect([&](SensorFaillure reading) {
        ESP_LOGE(TAG, "Sensor Error: %d : %d : %s", reading.type, reading.fatal, reading.errorMessage.c_str());
        sensorFaillures++;
    });
}

LatestReadings SensorRepository::getLatestReadings() { return this->latestReadings; }

AirQualityMeasurments SensorRepository::getLatestLevels() {
    return this->latestLevels;
}

int SensorRepository::getSensorFaillures() { return this->sensorFaillures; }