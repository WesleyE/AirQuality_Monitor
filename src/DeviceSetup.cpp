#include "DeviceSetup.h"

#include <Preferences.h>
#include <Wire.h>

#include "WifiProvision.h"
#include "common.h"
#include "helpers.h"

#define LOG_BUFFER_SIZE (1024)

static const char* TAG = "DEV_SETUP";
const char* ntpServer = "pool.ntp.org";

WifiProvision provisioner;

void DeviceSetup::setup() {
    ESP_LOGI(TAG, "Device Setup");
    DeviceSetup::setupPins();
    DeviceSetup::setupNVS();
    DeviceSetup::setupProvisioning();
    DeviceSetup::setupTime();
}

void DeviceSetup::setupPins() {
    ESP_LOGI(TAG, "Setting up Pins");

    // Some pins are stick after a software reset:
    pinMode(PIN_BTN_BOOT, OUTPUT);
    digitalWrite(PIN_BTN_BOOT, HIGH);

    vTaskDelay(200 / portTICK_RATE_MS);

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BTN_BOOT, INPUT);

    // Setup default states
    digitalWrite(PIN_LED, HIGH);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
}

void DeviceSetup::setupNVS() {
    ESP_LOGI(TAG, "NVS / Prefrences Setup");

    // First preference open should be read/write, since we need to write the namespace
    Preferences preferences;
    bool couldOpenPreferences = preferences.begin("preferences", false);
    if (!couldOpenPreferences) {
        ESP_LOGE(TAG, "Could not open NVS Preferences");
        abort();
    }

    // Init NVS, add new board ID
    bool tpInit = preferences.isKey("prefsInit");

    // Work-around for debugging
    if (tpInit == false) {
        ESP_LOGI(TAG, "NVS Init");
        preferences.clear();

        // 6 chars + null termination
        String serial = getSerialNumber();

        ESP_LOGI(TAG, "Setting new Board ID from mac: %s", serial);

        preferences.putString("boardID", serial);
        preferences.putBool("prefsInit", true);
        preferences.end();
        preferences.begin("preferences", true);
    }

    ESP_LOGI(TAG, "Board ID: %s", preferences.getString("boardID"));
    preferences.end();
}

void DeviceSetup::setupProvisioning() {
    // Blink the LED to signal factory reset when pushing boot button
    ESP_LOGI(TAG, "Press boot now to enter into provisioning mode...");
    bool state = HIGH;
    for (uint8_t i = 0; i < 12; i++) {
        digitalWrite(PIN_LED, !state);
        state = !state;
        vTaskDelay(200 / portTICK_RATE_MS);
    }

    digitalWrite(PIN_LED, LOW);

    // Make sure it is not a fluke
    int bootPinState = digitalRead(PIN_BTN_BOOT);
    vTaskDelay(1000 / portTICK_RATE_MS);
    int bootPinState2 = digitalRead(PIN_BTN_BOOT);

    // Flash LED to signal factory reset
    if (!bootPinState && !bootPinState2) {
        ESP_LOGI(TAG, "Reset requested...");
        bool state = LOW;
        for (uint8_t i = 0; i < 2; i++) {
            digitalWrite(PIN_LED, !state);
            state = !state;
            vTaskDelay(100 / portTICK_RATE_MS);
        }

        provisioner.forceReprovision();
    }

    digitalWrite(PIN_LED, HIGH);

    if (!provisioner.provisionWifi()) {
        ESP_LOGI(TAG, "Provisioning failed, restarting...");
        vTaskDelay(2000 / portTICK_RATE_MS);
        ESP.restart();
    }
    ESP_LOGI(TAG, "Provisioning done");
}

void DeviceSetup::setupTime() {
    ESP_LOGI(TAG, "Setting up time");
    configTime(0, 0, ntpServer);

    time_t now = getUnixTime();
    ESP_LOGW(TAG, "Current time: %ld", now);

    // setenv("TZ", "Europe/Amsterdam", 1);
    // tzset();
}