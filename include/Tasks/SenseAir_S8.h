#pragma once

#include "Tasks/Task.h"
#include "s8_uart.h"

struct SenseAirS8Reading {
    int16_t co2 = 0;
};

class SenseAir_S8 : public Task {
   public:
    void run(void *data);

    void setup();

   private:
    S8_UART *sensor_S8;
    S8_sensor sensor;

    bool tryConnect();
    void calibrate();
    bool shouldLogValues = false;
};