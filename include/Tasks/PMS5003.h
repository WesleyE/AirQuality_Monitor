#pragma once

#include "PMS.h"
#include "Tasks/Task.h"
#include "common.h"

struct PMS5003Reading {
    uint16_t Atmospheric_PM_1_0;
    uint16_t Atmospheric_PM_2_5;
    uint16_t Atmospheric_PM_10_0;
};

class PMS5003 : public Task {
   public:
    PMS5003(std::string taskName = "Task", uint16_t stackSize = 10000, uint8_t priority = 5) : Task(taskName, stackSize, priority), pms(Serial2){};
    void run(void *data);

   private:
    void setup();
    PMS pms;
    bool shouldLogValues = false;
};