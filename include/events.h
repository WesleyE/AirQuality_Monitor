#pragma once

#include <observe/event.h>

#include "AirQualityLevels.h"
#include "Tasks/BME280.h"
#include "Tasks/Logging.h"
#include "Tasks/PMS5003.h"
#include "Tasks/SGP41.h"
#include "Tasks/SenseAir_S8.h"
#include "Tasks/SensorRepository.h"
#include "Tasks/VEML7700.h"
#include "common.h"

extern observe::Event<VEML7700Reading> newVEML7700Reading;
extern observe::Event<BME280Reading> newBME280Reading;
extern observe::Event<SenseAirS8Reading> newSenseAirS8Reading;
extern observe::Event<SGP41Reading> newSGP41Reading;
extern observe::Event<PMS5003Reading> newPMS5003Reading;

extern observe::Event<> calibrateSenseAirS8;

extern observe::Event<SensorFaillure> sensorFaillure;
extern observe::Event<LatestReadings> sensorReport;
extern observe::Event<AirQualityMeasurments> sensorLevelsReport;

extern observe::Event<AQ_HttpLogConfig> logEvent;