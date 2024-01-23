#pragma once

#include <vector>

#include "AirQualityLevels.h"
#include "Tasks/BME280.h"
#include "Tasks/PMS5003.h"
#include "Tasks/SGP41.h"
#include "Tasks/SenseAir_S8.h"
#include "Tasks/Task.h"
#include "Tasks/VEML7700.h"

struct SensorFaillure {
    SensorType type;
    bool fatal = false;
    String errorMessage = "";
};

struct LatestReadings {
    VEML7700Reading latestVEML7700Reading;
    int64_t lastVEML7700ReadingTime;

    BME280Reading latestBME280Reading;
    int64_t lastBME280ReadingTime;

    SenseAirS8Reading latestSenseAirS8Reading;
    int64_t lastSenseAirS8ReadingTime;

    SGP41Reading latestSGP41Reading;
    int64_t lastSGP41ReadingTime;

    PMS5003Reading latestPMS5003Reading;
    int64_t lastPMS5003ReadingTime;
};

class SensorRepository : public Task {
   public:
    void run(void *data);
    void setup();

    LatestReadings getLatestReadings();
    AirQualityMeasurments getLatestLevels();
    int getSensorFaillures();

   private:
    LatestReadings latestReadings;
    int sensorFaillures = 0;
    AirQualityLevels levels;
    AirQualityMeasurments latestLevels;
};