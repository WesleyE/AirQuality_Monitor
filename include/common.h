#pragma once

#include <Arduino.h>

#include "Tasks/Logging.h"
#include "esp_log.h"
#include "pins.h"

#define AIRQUALITY_VERSION "0.6"

enum SensorType { VEML7700_SENSOR, SENSEAIR_S8_SENSOR, SGP41_SENSOR, PMS5003_SENSOR, BME280_SENSOR };