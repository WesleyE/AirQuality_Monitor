#pragma once

#include <SensirionI2CSgp41.h>
#include <Wire.h>

#include "Tasks/Task.h"

struct SGP41Reading {
    uint16_t rawNOx = 0;
    uint16_t rawVoc = 0;

    int32_t vocIndex = 0;
    int32_t noxIndex = 0;
};

class SGP41 : public Task {
   public:
    void run(void *data);

    void checkSelfTest();
    void checkSerialNumber();

   private:
    SensirionI2CSgp41 sgp41;

    // time in seconds needed for NOx conditioning
    uint16_t conditioning_s = 10;
    bool shouldLogValues = false;
};