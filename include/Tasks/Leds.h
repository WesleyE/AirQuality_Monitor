#pragma once

#include <Adafruit_NeoPixel.h>

#include "Tasks/Task.h"

class Leds : public Task {
   public:
    void run(void *data);
    void setup();

   private:
    Adafruit_NeoPixel pixel;
};