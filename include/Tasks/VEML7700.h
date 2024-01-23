#pragma once

#include "Adafruit_VEML7700.h"
#include "Tasks/Task.h"

struct VEML7700Reading {
    float lux;
};

class VEML7700 : public Task {
   public:
    void run(void *data);

    void setup();

   private:
    Adafruit_VEML7700 veml;
    bool shouldLogValues = false;
};