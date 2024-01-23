#include "WifiProvision.h"

#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>

#include "Tasks/HTTP.h"
#include "helpers.h"

static const char *TAG = "WIFI_PROV";

#define WIFI_CONNECTION_TIMEOUT 5000
#define DNS_PORT 53

extern const uint8_t provision_html_start[] asm("_binary_data_provision_html_start");
extern const uint8_t provision_html_end[] asm("_binary_data_provision_html_end");

extern HTTP http;

WifiProvision::WifiProvision() : softapIP(192, 168, 1, 1), netMask(255, 255, 255, 0) {}

bool WifiProvision::provisionWifi() {
    // If settings are not set, start AP
    if (!this->checkForSettings()) {
        ESP_LOGI(TAG, "No valid settings found, starting provisioning AP");
        return this->setupAP();
    }

    // Try to connect to the network
    if (!this->connectToWifi()) {
        return this->setupAP();
    }

    return true;
}

bool WifiProvision::forceReprovision() { return this->setupAP(); }

bool WifiProvision::checkForSettings() {
    Preferences preferences;
    preferences.begin("preferences", false);

    // Check if preferences are empty
    if (preferences.getString("wifiSsid", "") == "" || preferences.getString("wifiPassword", "") == "") {
        ESP_LOGI(TAG, "Empty settings");
        preferences.end();
        return false;
    }

    preferences.end();
    return true;
}

bool WifiProvision::connectToWifi() {
    Preferences preferences;
    preferences.begin("preferences", true);
    String storedSSID = preferences.getString("wifiSsid", "");
    String storedPassword = preferences.getString("wifiPassword", "");
    preferences.end();

    ESP_LOGI(TAG, "Connecting to WiFi");
    WiFi.mode(WIFI_STA);  // Set Wi-Fi mode to STA
    vTaskDelay(1000 / portTICK_RATE_MS);

    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());

    int64_t connectionStartedTime = 0;

    int64_t currentTime = getTimeSinceBootInSeconds();
    while (WiFi.status() != WL_CONNECTED && currentTime < connectionStartedTime + WIFI_CONNECTION_TIMEOUT) {
        currentTime = getTimeSinceBootInSeconds();
        vTaskDelay(500 / portTICK_RATE_MS);
    }

    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(TAG, "Could not connect to Wifi");
        return false;
    }

    ESP_LOGI(TAG, "WiFi connected");
    return true;
}

bool WifiProvision::setupAP() {
    ESP_LOGI(TAG, "Setting up AP");
    WiFi.mode(WIFI_AP_STA);
    vTaskDelay(1000 / portTICK_RATE_MS);

    Preferences preferences;
    preferences.begin("preferences");
    String softApNetworkName = "AirQuality - " + preferences.getString("boardID", "");
    preferences.end();

    WiFi.softAPConfig(softapIP, softapIP, netMask);
    WiFi.softAP(softApNetworkName.c_str());
    vTaskDelay(1000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Access point IP Address: %s", WiFi.softAPIP());

    ESP_LOGI(TAG, "Access point setup");

    this->setupHttpServer();
    this->setupDNSServer();

    while (true) {
        dnsServer.processNextRequest();
        vTaskDelay(200 / portTICK_RATE_MS);
    }

    return this->provisionWifi();
}

void WifiProvision::setupHttpServer() {
    http.isInProvisioningMode = true;
    http.start();
    ESP_LOGI(TAG, "HTTP server started");
}

void WifiProvision::setupDNSServer() {
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", this->softapIP);
}