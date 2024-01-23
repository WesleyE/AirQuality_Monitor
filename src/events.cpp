#include "events.h"

observe::Event<BME280Reading> newBME280Reading;
observe::Event<SenseAirS8Reading> newSenseAirS8Reading;
observe::Event<VEML7700Reading> newVEML7700Reading;
observe::Event<SGP41Reading> newSGP41Reading;
observe::Event<PMS5003Reading> newPMS5003Reading;

observe::Event<> calibrateSenseAirS8;

observe::Event<SensorFaillure> sensorFaillure;
observe::Event<LatestReadings> sensorReport;
observe::Event<AirQualityMeasurments> sensorLevelsReport;

observe::Event<AQ_HttpLogConfig> logEvent;