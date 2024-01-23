#pragma once

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "AirQualityLevels.h"
#include "Tasks/SensorRepository.h"
#include "Tasks/Task.h"

class MQTT : public Task {
   public:
    MQTT();
    void run(void* data);
    void setup();

   private:
    WiFiClient espClient;
    PubSubClient client;
    String clientId;

    String broker;
    String username;
    String password;
    String boardId;

    void reconnect();
    void sendJsonToTopic(const char* topic, String json);
    void sendDeviceConfiguration();

    void reloadConfiguration();

    void sendMeasurementResults(LatestReadings results);
    void sendLevelMeasurementResults(AirQualityMeasurments levelMeasurements);

    DynamicJsonDocument getDeviceConfigurationTemplate();
    void sendDeviceTemplate(const char* uniqueId, const char* name, const char* deviceClass, const char* unitOfMeasurement, const char* valueTemplate,
                            const char* stateTopic = "");
};