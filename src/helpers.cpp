#include "helpers.h"

#include <Preferences.h>
#include <stdio.h>

#include <iostream>
#include <stdexcept>

#include "WiFi.h"
#include "WiFiGeneric.h"
#include "WiFiSTA.h"
#include "esp_random.h"
#include "esp_wifi.h"

/**
 * @brief Get the Time Since Boot In Seconds
 *
 * @return int64_t
 */
int64_t getTimeSinceBootInSeconds() { return esp_timer_get_time() / 1000000; }

/**
 * @brief Get the Serial Number from the last 3 segments of the MAC
 *
 */
String getSerialNumber() {
    uint8_t mac[6];
    char macStr[18] = {0};
    if (WiFiGenericClass::getMode() == WIFI_MODE_NULL) {
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
    } else {
        esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, mac);
    }
    sprintf(macStr, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(macStr);
}

/**
 * @brief Get a random string of x length
 *
 * @param output
 * @param len
 */
void getRandomStr(char* output, int len) {
    char const* eligible_chars = "abcdef0123456789";
    for (int i = 0; i < (len - 1); i++) {
        uint32_t rand_int = std::abs((int)esp_random());
        int random_index = std::max((double)((uint64_t)rand_int * strlen(eligible_chars) * 1.0) / 4294967296 - 1, (double)0);
        output[i] = eligible_chars[random_index];
    }

    output[len - 1] = '\0';
}

/**
 * @brief Get the Unix Time object
 *
 * @return time_t
 */
time_t getUnixTime() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        ESP_LOGW("TIME", "Failed to obtain time");
    }
    time(&now);

    return now;
}

/**
 * @brief Grab the value in a separated string by index
 *
 * @param data
 * @param separator
 * @param index
 * @return String
 */
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool getShouldLogValues() {
    Preferences preferences;
    preferences.begin("preferences", true);
    bool shouldLogValues = preferences.getBool("logValues", false);
    preferences.end();
    return shouldLogValues;
}