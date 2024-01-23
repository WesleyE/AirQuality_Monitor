#include "Tasks/Leds.h"

#include <Preferences.h>

#include "LogMacros.h"
#include "common.h"
#include "events.h"

static const char *TAG = "LEDS";

void Leds::run(void *data) {
    ESP_LOGI(TAG, "Task LEDS started");

    setup();

    Preferences preferences;
    preferences.begin("preferences", true);
    uint16_t ledIntensity = preferences.getUInt("ledIntensity", 1);
    preferences.end();

    sensorLevelsReport.connect([&](AirQualityMeasurments levelMeasurements) {
        uint32_t color = pixel.Color(0, 0, 0);
        if(ledIntensity == 0) {
            pixel.setPixelColor(0, color);
            return;
        }
        
        switch (levelMeasurements.calculatedLevel) {
            case AirQualityLevel::GOOD:
                color = pixel.Color(0 / ledIntensity, 255 / ledIntensity, 0);  // Green
                break;
            case AirQualityLevel::FAIR:
                color = pixel.Color(128 / ledIntensity, 255 / ledIntensity, 0);  // Yellow-Green
                break;
            case AirQualityLevel::MODERATE:
                color = pixel.Color(255 / ledIntensity, 255 / ledIntensity, 0);  // Yellow
                break;
            case AirQualityLevel::POOR:
                color = pixel.Color(255 / ledIntensity, 128 / ledIntensity, 0);  // Orange
                break;
            case AirQualityLevel::VERY_POOR:
                color = pixel.Color(255 / ledIntensity, 0 / ledIntensity, 0);  // Red
                break;
            case AirQualityLevel::EXTREMELY_POOR:
                color = pixel.Color(255 / ledIntensity, 0 / ledIntensity, 0);  // Red
                break;
            default:
                break;
        }

        pixel.setPixelColor(0, color);
        pixel.show();
    });

    for (;;) {
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
}

void Leds::setup() {
    pixel = Adafruit_NeoPixel(1, PIN_RGB_LED);
    pixel.begin();

    pixel.clear();
}