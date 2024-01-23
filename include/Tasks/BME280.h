#pragma once

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

#include "Tasks/Task.h"

struct BME280Reading {
    float temperature = 0.0f;
    float pressure = 0.0f;
    float humidity = 0.0f;
};

class BME280 : public Task {
   public:
    void run(void *data);

    void setup();

   private:
    Adafruit_BME280 bme;
    bool shouldLogValues = false;
};