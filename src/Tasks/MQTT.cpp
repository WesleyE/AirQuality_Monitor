#include "Tasks/MQTT.h"

#include <Preferences.h>

#include "LogMacros.h"
#include "common.h"
#include "events.h"
#include "helpers.h"

static const char* TAG = "MQTT";

const int mqtt_port = 1883;

MQTT::MQTT() : client(this->espClient) {}

void MQTT::run(void* data) {
    setup();

    for (;;) {
        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void MQTT::setup() {
    ESP_LOGI(TAG, "Task MQTT started");
    client.setBufferSize(2048);

    Preferences preferences;
    preferences.begin("preferences", true);

    broker = preferences.getString("mqttHost");
    username = preferences.getString("mqttUsername");
    password = preferences.getString("mqttPassword");
    boardId = preferences.getString("boardID", "000000");

    if (broker == "") {
        ESP_LOGE(TAG, "No broker set, not trying to connect.");
        for (;;) {
        }
    }

    this->clientId = "airquality_3_" + boardId;
    ESP_LOGI(TAG, "Client ID: %s", this->clientId.c_str());
    client.setServer(broker.c_str(), 1883);

    preferences.end();

    this->reconnect();
    this->sendDeviceConfiguration();

    sensorReport.connect([&](LatestReadings readings) { this->sendMeasurementResults(readings); });
    sensorLevelsReport.connect([&](AirQualityMeasurments levelMeasurements) { this->sendLevelMeasurementResults(levelMeasurements); });
}

void MQTT::sendLevelMeasurementResults(AirQualityMeasurments levelMeasurements) {
    ESP_LOGI(TAG, "Sending levels");
    DynamicJsonDocument doc(2048);

    doc["NOxLevel"] = AirQualityLevels::airQualityLevelToString(levelMeasurements.NOxLevel);
    doc["VOCLevel"] = AirQualityLevels::airQualityLevelToString(levelMeasurements.VOCLevel);
    doc["CO2Level"] = AirQualityLevels::airQualityLevelToString(levelMeasurements.CO2Level);
    doc["PM2_5Level"] = AirQualityLevels::airQualityLevelToString(levelMeasurements.PM2_5Level);
    doc["PM10Level"] = AirQualityLevels::airQualityLevelToString(levelMeasurements.PM10Level);
    doc["calculatedLevel"] = AirQualityLevels::airQualityLevelToString(levelMeasurements.calculatedLevel);

    String jsonOutput = "";
    serializeJson(doc, jsonOutput);
    sendJsonToTopic(("airquality/" + this->clientId + "/state/levels").c_str(), jsonOutput);
}

void MQTT::sendMeasurementResults(LatestReadings results) {
    ESP_LOGI(TAG, "Sending readings");

    DynamicJsonDocument doc(2048);

    doc["temperature"] = results.latestBME280Reading.temperature;
    doc["pressure"] = results.latestBME280Reading.pressure;
    doc["humidity"] = results.latestBME280Reading.humidity;

    doc["co2"] = results.latestSenseAirS8Reading.co2;

    doc["nox"] = results.latestSGP41Reading.rawNOx;
    doc["voc"] = results.latestSGP41Reading.rawVoc;
    doc["aqi_voc"] = results.latestSGP41Reading.vocIndex;
    doc["aqi_nox"] = results.latestSGP41Reading.noxIndex;

    doc["pm1"] = results.latestPMS5003Reading.Atmospheric_PM_1_0;
    doc["pm25"] = results.latestPMS5003Reading.Atmospheric_PM_2_5;
    doc["pm10"] = results.latestPMS5003Reading.Atmospheric_PM_10_0;

    doc["lux"] = results.latestVEML7700Reading.lux;

    String jsonOutput = "";
    serializeJson(doc, jsonOutput);
    sendJsonToTopic(("airquality/" + this->clientId + "/state").c_str(), jsonOutput);
}

void MQTT::reconnect() {
    while (!client.connected()) {
        ESP_LOGI(TAG, "Attempting MQTT connection...");

        // Attempt to connect
        if (client.connect(this->clientId.c_str(), username.c_str(), password.c_str())) {
            ESP_LOGI(TAG, "Connected");
        } else {
            ESP_LOGE(TAG, "Failed, rc=%d. Trying again in 2 seconds.", client.state());
            // Wait 5 seconds before retrying
            vTaskDelay(2000 / portTICK_RATE_MS);
        }
    }
}

void MQTT::sendJsonToTopic(const char* topic, String json) {
    bool isPublished = client.publish(topic, json.c_str(), true);
    // ESP_LOGD(TAG, "Sending to %s (%d):\n%s\n\n", topic, isPublished, json.c_str());
}

DynamicJsonDocument MQTT::getDeviceConfigurationTemplate() {
    DynamicJsonDocument doc(2048);
    doc["enabled_by_default"] = true;
    doc["state_class"] = "measurement";
    doc["device_class"] = "humidity";  // Sets the icon in the UI
    doc["state_topic"] = "airquality/" + this->clientId + "/state";
    doc["unit_of_measurement"] = "%";
    doc["value_template"] = "{{ value_json.humidity}}";
    doc["unique_id"] = "hum01ae";

    JsonObject device = doc.createNestedObject("device");
    device["identifiers"][0] = this->clientId.c_str();
    device["manufacturer"] = "Wesley Elfring";
    device["model"] = "Air Quality V3";
    device["name"] = "Air Quality Sensor";
    device["sw_version"] = "3";

    return doc;
}

void MQTT::sendDeviceConfiguration() {
    ESP_LOGI(TAG, "Sending Device Configuration...");

    sendDeviceTemplate("temperature", "Temperature", "temperature", "°C", "{{value_json.temperature}}");
    sendDeviceTemplate("humidity", "Humidity", "humidity", "%", "{{value_json.humidity}}");
    sendDeviceTemplate("pressure", "Pressure", "atmospheric_pressure", "Pa", "{{value_json.pressure}}");

    sendDeviceTemplate("co2", "CO2", "carbon_dioxide", "ppm", "{{value_json.co2}}");

    sendDeviceTemplate("nox", "NOx", "nitrous_oxide", "µg/m³", "{{value_json.nox}}");
    sendDeviceTemplate("voc", "VOC", "volatile_organic_compounds", "µg/m³", "{{value_json.voc}}");

    sendDeviceTemplate("lux", "Lux", "lx", "µg/m³", "{{value_json.lux}}");

    sendDeviceTemplate("pm1", "PM1", "pm1", "µg/m³", "{{value_json.pm1}}");
    sendDeviceTemplate("pm25", "PM2.5", "pm25", "µg/m³", "{{value_json.pm25}}");
    sendDeviceTemplate("pm10", "PM10", "pm10", "µg/m³", "{{value_json.pm10}}");

    sendDeviceTemplate("aqi_nox", "AQI NOx", "aqi", "", "{{value_json.aqi_nox}}");
    sendDeviceTemplate("aqi_voc", "AQI VOC", "aqi", "", "{{value_json.aqi_voc}}");

    // Send levels
    sendDeviceTemplate("NOxLevel", "NOx Level", "enum", "", "{{value_json.NOxLevel}}", "levels");
    sendDeviceTemplate("VOCLevel", "VOC Level", "enum", "", "{{value_json.VOCLevel}}", "levels");
    sendDeviceTemplate("CO2Level", "CO2 Level", "enum", "", "{{value_json.CO2Level}}", "levels");
    sendDeviceTemplate("PM2_5Level", "PM2.5 Level", "enum", "", "{{value_json.PM2_5Level}}", "levels");
    sendDeviceTemplate("PM10Level", "PM10 Level", "enum", "", "{{value_json.PM10Level}}", "levels");
    sendDeviceTemplate("calculatedLevel", "Calculated Level", "enum", "", "{{value_json.calculatedLevel}}", "levels");
}

void MQTT::sendDeviceTemplate(const char* uniqueId, const char* name, const char* deviceClass, const char* unitOfMeasurement,
                              const char* valueTemplate, const char* stateTopic) {
    DynamicJsonDocument doc = getDeviceConfigurationTemplate();

    doc = getDeviceConfigurationTemplate();
    doc["device_class"] = deviceClass;
    doc["unit_of_measurement"] = unitOfMeasurement;
    doc["value_template"] = valueTemplate;

    size_t totalLength = this->clientId.length() + 2 + strlen(uniqueId);  // Calculate the total length, adding space for the null terminator
    char* temp = new char[totalLength];                                   // Allocate temporary memory to hold the concatenated string
    strcpy(temp, this->clientId.c_str());                                 // Copy this->clientId to the temporary buffer
    strcat(temp, "_");                                                    // Concatenate underscore
    strcat(temp, uniqueId);                                               // Concatenate uniqueId
    doc["unique_id"] = temp;

    ESP_LOGD(TAG, "Sending config for %s // %s", uniqueId, temp);
    doc["name"] = name;

    if (stateTopic != "") {
        doc["state_topic"] = "airquality/" + this->clientId + "/state" + "/" + stateTopic;
    }

    String jsonOutput = "";
    serializeJson(doc, jsonOutput);
    ESP_LOGD(TAG, "Sending output: %s", jsonOutput.c_str());
    sendJsonToTopic(("homeassistant/sensor/" + this->clientId + "/" + uniqueId + "/config").c_str(), jsonOutput);
    doc.garbageCollect();
    delete[] temp;
}