#include "Tasks/HTTP.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_flash_partitions.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include <algorithm>
#include <functional>

#include "LogMacros.h"
#include "common.h"
#include "events.h"
#include "helpers.h"

using namespace std::placeholders;

static const char *TAG = "HTTP";

extern const uint8_t index_html_start[] asm("_binary_data_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_data_index_html_end");

extern const uint8_t settings_html_start[] asm("_binary_data_settings_html_start");
extern const uint8_t settings_html_end[] asm("_binary_data_settings_html_end");

extern const uint8_t javascript_js_start[] asm("_binary_data_javascript_js_start");
extern const uint8_t javascript_js_end[] asm("_binary_data_javascript_js_end");

#define MAX_POST_DATA_BUFFER 1024
#define WIFI_CONNECTION_TIMEOUT 5000

HTTP::HTTP(SensorRepository *repository) { this->repository = repository; }

void HTTP::run(void *data) {
    setup();

    for (;;) {
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void HTTP::setup() {
    Preferences preferences;
    preferences.begin("preferences", true);
    String hostname = "airquality-3-" + preferences.getString("boardID", "000000");
    preferences.end();

    if (MDNS.begin(hostname)) {
        ESP_LOGI(TAG, "MDNS responder started, hostname: %s", hostname);
    }

    ESP_LOGI(TAG, "HTTP server started");

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.stack_size = 20480;
    config.max_uri_handlers = 15;

    static const httpd_uri_t root = {.uri = "/",
                                     .method = HTTP_GET,
                                     .handler =
                                         [](httpd_req_t *req) {
                                             HTTP *httpServer = (HTTP *)req->user_ctx;
                                             return httpServer->handleIndex(req);
                                         },
                                     .user_ctx = this};

    static const httpd_uri_t provisioningRoot = {.uri = "/",
                                                 .method = HTTP_GET,
                                                 .handler =
                                                     [](httpd_req_t *req) {
                                                         HTTP *httpServer = (HTTP *)req->user_ctx;
                                                         return httpServer->handleProvisioningIndex(req);
                                                     },
                                                 .user_ctx = this};

    static const httpd_uri_t sensorValues = {.uri = "/sensorvalues",
                                             .method = HTTP_GET,
                                             .handler =
                                                 [](httpd_req_t *req) {
                                                     HTTP *httpServer = (HTTP *)req->user_ctx;
                                                     return httpServer->handleSensorValues(req);
                                                 },
                                             .user_ctx = this};

    static const httpd_uri_t sensorValuesOptions = {.uri = "/sensorvalues",
                                                    .method = HTTP_OPTIONS,
                                                    .handler =
                                                        [](httpd_req_t *req) {
                                                            HTTP *httpServer = (HTTP *)req->user_ctx;
                                                            return httpServer->handleOptions(req);
                                                        },
                                                    .user_ctx = this};

    static const httpd_uri_t settingsGet = {.uri = "/settings",
                                            .method = HTTP_GET,
                                            .handler =
                                                [](httpd_req_t *req) {
                                                    HTTP *httpServer = (HTTP *)req->user_ctx;
                                                    return httpServer->handleGetSettings(req);
                                                },
                                            .user_ctx = this};

    static const httpd_uri_t preferencesGet = {.uri = "/preferences",
                                               .method = HTTP_GET,
                                               .handler =
                                                   [](httpd_req_t *req) {
                                                       HTTP *httpServer = (HTTP *)req->user_ctx;
                                                       return httpServer->handleGetPreferences(req);
                                                   },
                                               .user_ctx = this};

    static const httpd_uri_t preferencesOptions = {.uri = "/preferences",
                                                   .method = HTTP_OPTIONS,
                                                   .handler =
                                                       [](httpd_req_t *req) {
                                                           HTTP *httpServer = (HTTP *)req->user_ctx;
                                                           return httpServer->handleOptions(req);
                                                       },
                                                   .user_ctx = this};

    static const httpd_uri_t preferencesPost = {.uri = "/preferences",
                                                .method = HTTP_POST,
                                                .handler =
                                                    [](httpd_req_t *req) {
                                                        HTTP *httpServer = (HTTP *)req->user_ctx;
                                                        return httpServer->handlePostPreferences(req);
                                                    },
                                                .user_ctx = this};

    static const httpd_uri_t networks = {.uri = "/networks",
                                         .method = HTTP_GET,
                                         .handler =
                                             [](httpd_req_t *req) {
                                                 HTTP *httpServer = (HTTP *)req->user_ctx;
                                                 return httpServer->handleGetNetworks(req);
                                             },
                                         .user_ctx = this};

    static const httpd_uri_t reset = {.uri = "/reset",
                                      .method = HTTP_POST,
                                      .handler =
                                          [](httpd_req_t *req) {
                                              HTTP *httpServer = (HTTP *)req->user_ctx;
                                              return httpServer->handlePostReset(req);
                                          },
                                      .user_ctx = this};

    static const httpd_uri_t calibrate = {.uri = "/calibrate",
                                          .method = HTTP_POST,
                                          .handler =
                                              [](httpd_req_t *req) {
                                                  HTTP *httpServer = (HTTP *)req->user_ctx;
                                                  return httpServer->handlePostCalibrate(req);
                                              },
                                          .user_ctx = this};

    static const httpd_uri_t javascript = {.uri = "/javascript.js",
                                           .method = HTTP_GET,
                                           .handler =
                                               [](httpd_req_t *req) {
                                                   HTTP *httpServer = (HTTP *)req->user_ctx;
                                                   return httpServer->handleGetJavascript(req);
                                               },
                                           .user_ctx = this};

    static const httpd_uri_t status = {.uri = "/status",
                                       .method = HTTP_GET,
                                       .handler =
                                           [](httpd_req_t *req) {
                                               HTTP *httpServer = (HTTP *)req->user_ctx;
                                               return httpServer->handleGetStatus(req);
                                           },
                                       .user_ctx = this};

    static const httpd_uri_t ota = {.uri = "/ota",
                                    .method = HTTP_POST,
                                    .handler =
                                        [](httpd_req_t *req) {
                                            HTTP *httpServer = (HTTP *)req->user_ctx;
                                            return httpServer->handlePostOTA(req);
                                        },
                                    .user_ctx = this};

    static const httpd_uri_t resetOta = {.uri = "/ota/reset",
                                    .method = HTTP_POST,
                                    .handler =
                                        [](httpd_req_t *req) {
                                            HTTP *httpServer = (HTTP *)req->user_ctx;
                                            return httpServer->handlePostResetOTA(req);
                                        },
                                    .user_ctx = this};



    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (isInProvisioningMode) {
        ESP_LOGI(TAG, "HTTP server is in provisioning mode");
    }
    if (httpd_start(&server, &config) == ESP_OK) {
        if (!isInProvisioningMode) {
            httpd_register_uri_handler(server, &root);
            httpd_register_uri_handler(server, &sensorValues);
            httpd_register_uri_handler(server, &sensorValuesOptions);
            httpd_register_uri_handler(server, &status);
        } else {
            /*
                Turn of warnings from HTTP server as redirecting traffic will yield
                lots of invalid requests
            */
            esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
            esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
            esp_log_level_set("httpd_parse", ESP_LOG_ERROR);

            httpd_register_uri_handler(server, &provisioningRoot);
        }

        httpd_register_uri_handler(server, &settingsGet);
        httpd_register_uri_handler(server, &preferencesGet);
        httpd_register_uri_handler(server, &preferencesOptions);
        httpd_register_uri_handler(server, &preferencesPost);

        httpd_register_uri_handler(server, &networks);
        httpd_register_uri_handler(server, &reset);
        httpd_register_uri_handler(server, &calibrate);
        httpd_register_uri_handler(server, &javascript);
        httpd_register_uri_handler(server, &ota);
        httpd_register_uri_handler(server, &resetOta);

        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, [](httpd_req_t *req, httpd_err_code_t err) {
            HTTP *httpServer = (HTTP *)req->user_ctx;
            return httpServer->httpErrorHandler(req, err);
        });

        ESP_LOGI(TAG, "HTTP server started");
    }

    MDNS.addService("http", "tcp", 80);
}

esp_err_t HTTP::handleIndex(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HTTP::handleGetSettings(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)settings_html_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HTTP::handleSensorValues(httpd_req_t *req) {
    LatestReadings readings = this->repository->getLatestReadings();
    AirQualityMeasurments levels = this->repository->getLatestLevels();

    DynamicJsonDocument doc(2048);

    doc["temperature"] = readings.latestBME280Reading.temperature;
    doc["pressure"] = readings.latestBME280Reading.pressure;
    doc["humidity"] = readings.latestBME280Reading.humidity;

    doc["co2"] = readings.latestSenseAirS8Reading.co2;

    doc["nox"] = readings.latestSGP41Reading.rawNOx;
    doc["voc"] = readings.latestSGP41Reading.rawVoc;
    doc["aqi_voc"] = readings.latestSGP41Reading.vocIndex;
    doc["aqi_nox"] = readings.latestSGP41Reading.noxIndex;

    doc["pm1"] = readings.latestPMS5003Reading.Atmospheric_PM_1_0;
    doc["pm25"] = readings.latestPMS5003Reading.Atmospheric_PM_2_5;
    doc["pm10"] = readings.latestPMS5003Reading.Atmospheric_PM_10_0;

    doc["lux"] = readings.latestVEML7700Reading.lux;

    doc["NOxLevel"] = AirQualityLevels::airQualityLevelToString(levels.NOxLevel);
    doc["VOCLevel"] = AirQualityLevels::airQualityLevelToString(levels.VOCLevel);
    doc["CO2Level"] = AirQualityLevels::airQualityLevelToString(levels.CO2Level);
    doc["PM2_5Level"] = AirQualityLevels::airQualityLevelToString(levels.PM2_5Level);
    doc["PM10Level"] = AirQualityLevels::airQualityLevelToString(levels.PM10Level);
    doc["calculatedLevel"] = AirQualityLevels::airQualityLevelToString(levels.calculatedLevel);

    String jsonOutput = "";
    serializeJson(doc, jsonOutput);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, jsonOutput.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HTTP::handleProvisioningIndex(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)settings_html_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HTTP::handleGetPreferences(httpd_req_t *req) {
    DynamicJsonDocument doc(2048);

    Preferences preferences;
    preferences.begin("preferences", true);
    doc["wifiSsid"] = preferences.getString("wifiSsid");
    doc["deviceName"] = preferences.getString("deviceName");
    doc["deviceVersion"] = preferences.getString("deviceVersion");
    doc["mqttHost"] = preferences.getString("mqttHost");
    doc["mqttUsername"] = preferences.getString("mqttUsername");
    doc["logHost"] = preferences.getString("logHost");
    doc["logUsername"] = preferences.getString("logUsername");
    doc["logValues"] = preferences.getBool("logValues");
    doc["ledIntensity"] = preferences.getUInt("ledIntensity");

    doc["provisioningMode"] = this->isInProvisioningMode;

    // Do not send actual passwords, but do show when no password has been set
    if (preferences.getString("wifiPassword") == "") {
        doc["wifiPassword"] = "";
    } else {
        doc["wifiPassword"] = "****";
    }

    if (preferences.getString("mqttPassword") == "") {
        doc["mqttPassword"] = "";
    } else {
        doc["mqttPassword"] = "****";
    }

    if (preferences.getString("logPassword") == "") {
        doc["logPassword"] = "";
    } else {
        doc["logPassword"] = "****";
    }

    preferences.end();

    String jsonOutput = "";
    serializeJson(doc, jsonOutput);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, jsonOutput.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HTTP::handlePostPreferences(httpd_req_t *req) {
    ESP_LOGI(TAG, "Post started");

    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, this->getPostData(req), MAX_POST_DATA_BUFFER);
    if (error) {
        ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
        return ESP_FAIL;
    }

    // Store preferences
    Preferences preferences;
    preferences.begin("preferences", false);

    preferences.putString("wifiSsid", doc["wifiSsid"].as<String>());
    preferences.putString("deviceName", doc["deviceName"].as<String>());
    preferences.putString("deviceVersion", doc["deviceVersion"].as<String>());
    preferences.putString("mqttHost", doc["mqttHost"].as<String>());
    preferences.putString("mqttUsername", doc["mqttUsername"].as<String>());
    preferences.putString("logHost", doc["logHost"].as<String>());
    preferences.putString("logUsername", doc["logUsername"].as<String>());
    preferences.putUInt("ledIntensity", doc["ledIntensity"].as<uint16_t>());

    preferences.putBool("logValues", doc["logValues"].as<bool>());

    // Do not update the password when the string is only the placeholder asterisks
    if (doc["wifiPassword"].as<String>() != "****") {
        preferences.putString("wifiPassword", doc["wifiPassword"].as<String>());
    }

    if (doc["mqttPassword"].as<String>() != "****") {
        preferences.putString("mqttPassword", doc["mqttPassword"].as<String>());
    }

    if (doc["logPassword"].as<String>() != "****") {
        preferences.putString("logPassword", doc["logPassword"].as<String>());
    }

    // We are only allowed to change the device version during provisioning
    if (this->isInProvisioningMode) {
        preferences.putString("deviceVersion", doc["deviceVersion"].as<String>());
    }
    preferences.end();

    ESP_LOGI(TAG, "Settings saved");

    if (this->isInProvisioningMode) {
        ESP_LOGI(TAG, "Checking for WiFi connectivity");

        // Check for valid details
        WiFi.disconnect();
        vTaskDelay(1000 / portTICK_RATE_MS);

        ESP_LOGI(TAG, "Connecting with: %s - %s", doc["wifiSsid"].as<String>(), doc["wifiPassword"].as<String>());

        WiFi.begin(doc["wifiSsid"].as<String>().c_str(), doc["wifiPassword"].as<String>().c_str());

        int64_t connectionStartedTime = getTimeSinceBootInSeconds();
        int64_t currentTime = getTimeSinceBootInSeconds();
        while (WiFi.status() != WL_CONNECTED && currentTime < connectionStartedTime + WIFI_CONNECTION_TIMEOUT) {
            ESP_LOGI(TAG, "Wifi status: %d", WiFi.status());

            currentTime = getTimeSinceBootInSeconds();
            vTaskDelay(500 / portTICK_RATE_MS);
        }

        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGE(TAG, "Could not connect to Wifi");
            WiFi.disconnect();
            vTaskDelay(1000 / portTICK_RATE_MS);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_sendstr(req, "{\"error\": \"Could not connect to network.\"}");
            return ESP_OK;
        }

        httpd_resp_sendstr(req, "Wifi connection was successfull.");
        ESP_LOGI(TAG, "Wifi connection was successfull, restarting in 5 seconds.");
        vTaskDelay(5000 / portTICK_RATE_MS);
        ESP.restart();
        return ESP_OK;
    }

    return ESP_OK;
}

esp_err_t HTTP::handleGetNetworks(httpd_req_t *req) {
    String networkJson = this->getWifiNetworks();

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, networkJson.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

const char *HTTP::getPostData(httpd_req_t *req) {
    static char buf[MAX_POST_DATA_BUFFER];
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    if (total_len >= MAX_POST_DATA_BUFFER) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        ESP_LOGE(TAG, "Content too long");
        return "";
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            ESP_LOGE(TAG, "Failed to post control value");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return "";
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    ESP_LOGI(TAG, "Post data: %s", buf);
    return buf;
}

esp_err_t HTTP::handleOptions(httpd_req_t *req) {
    httpd_resp_set_status(req, HTTPD_200);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, x-requested-with");

    httpd_resp_send(req, "", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t HTTP::httpErrorHandler(httpd_req_t *req, httpd_err_code_t err) {
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root from %s -> %d", req->uri, req->method);
    return ESP_OK;
}

String HTTP::getWifiNetworks() {
    //  Get Availible networks and return as json
    StaticJsonDocument<4096> jsonDoc;
    JsonArray networks = jsonDoc.to<JsonArray>();

    ESP_LOGI(TAG, "Starting new network scan");

    int numberOfFoundNetworks = WiFi.scanNetworks(false, false);
    if (numberOfFoundNetworks == 0) {
        ESP_LOGI(TAG, "No networks found");
        return "[]";
    }

    for (int i = 0; i < numberOfFoundNetworks; i++) {
        JsonObject netNetwork = networks.createNestedObject();
        netNetwork["ssid"] = WiFi.SSID(i);
        netNetwork["channel"] = WiFi.channel(i);
        netNetwork["rssi"] = WiFi.RSSI(i);
        netNetwork["authmode"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "SECURED";
    }

    ESP_LOGI(TAG, "Found %d networks", numberOfFoundNetworks);
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    return jsonString;
}

esp_err_t HTTP::handlePostReset(httpd_req_t *req) {
    httpd_resp_sendstr(req, "Reset requested from HTTP interface.");
    ESP_LOGI(TAG, "Reset requested from HTTP interface.");
    vTaskDelay(5000 / portTICK_RATE_MS);
    ESP.restart();
    return ESP_OK;
}

esp_err_t HTTP::handlePostCalibrate(httpd_req_t *req) {
    httpd_resp_sendstr(req, "Callibration started, starting in 5 seconds.");
    ESP_LOGI(TAG, "Callibration started, starting in 5 seconds.");
    vTaskDelay(5000 / portTICK_RATE_MS);
    calibrateSenseAirS8.emit();
    return ESP_OK;
}

esp_err_t HTTP::handleGetJavascript(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(req, (const char *)javascript_js_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HTTP::handleGetStatus(httpd_req_t *req) {
    StaticJsonDocument<512> doc;

    JsonObject last_sensor_reads = doc.createNestedObject("last_sensor_reads");

    LatestReadings readings = this->repository->getLatestReadings();

    doc["sensor_id"] = getSerialNumber();
    Preferences preferences;
    preferences.begin("preferences", true);
    doc["sensor_name"] = preferences.getString("deviceName", "");
    preferences.end();

    last_sensor_reads["VEML7700"] = readings.lastVEML7700ReadingTime;
    last_sensor_reads["BME280"] = readings.lastBME280ReadingTime;
    last_sensor_reads["SGP41"] = readings.lastSGP41ReadingTime;
    last_sensor_reads["S8"] = readings.lastSenseAirS8ReadingTime;
    last_sensor_reads["PMS5003"] = readings.lastPMS5003ReadingTime;
    doc["total_sensor_faillures"] = this->repository->getSensorFaillures();
    doc["seconds_since_boot"] = getTimeSinceBootInSeconds();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["total_heap"] = ESP.getHeapSize();
    doc["version"] = AIRQUALITY_VERSION;

    String output;
    serializeJson(doc, output);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, output.c_str(), HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

esp_err_t HTTP::handlePostResetOTA(httpd_req_t *req) {
    httpd_resp_sendstr(req, "Reset OTA requested from HTTP interface.");
    ESP_LOGI(TAG, "Reset OTA requested from HTTP interface.");
    vTaskDelay(5000 / portTICK_RATE_MS);
    esp_ota_mark_app_invalid_rollback_and_reboot();
    return ESP_OK;
}

esp_err_t HTTP::handlePostOTA(httpd_req_t *req) {
    httpd_resp_set_status(req, HTTPD_500);  // Assume failure

    int ret, remaining = req->content_len;

    ESP_LOGI(TAG, "Receiving OTA of size %d.", ret);

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (update_partition == NULL) {
        httpd_resp_sendstr(req, "Could not get partition");
        ESP_LOGE(TAG, "Could not get partition %d.", ret);
        return ESP_FAIL;
    }

    esp_err_t err = ESP_OK;
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        httpd_resp_sendstr(req, "Could not start OTA");
        ESP_LOGE(TAG, "Could not start OTA: %s.", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        return ESP_FAIL;
    }

    char buf[256];

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // Retry receiving if timeout occurred
                continue;
            }
            // Read was not successfull
            httpd_resp_sendstr(req, "Could not read from stream");
            ESP_LOGE(TAG, "Could not read from stream.");
            esp_ota_abort(update_handle);
            return ESP_FAIL;
        }

        size_t bytes_read = ret;
        remaining -= bytes_read;
        err = esp_ota_write(update_handle, buf, bytes_read);
        if (err != ESP_OK) {
            httpd_resp_sendstr(req, "Could not write OTA");
            ESP_LOGE(TAG, "Could not write OTA: %s.", esp_err_to_name(err));
            esp_ota_abort(update_handle);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "OTA receive done");

    if ((esp_ota_end(update_handle) == ESP_OK) && (esp_ota_set_boot_partition(update_partition) == ESP_OK)) {
        ESP_LOGI(TAG, "OTA successfull, rebooting in 5 seconds.");
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_send(req, NULL, 0);
        vTaskDelay(5000 / portTICK_RATE_MS);
        esp_restart();

        return ESP_OK;
    }

    esp_ota_abort(update_handle);
    ESP_LOGE(TAG, "OTA Failed: %s.", esp_err_to_name(err));
    httpd_resp_send(req, NULL, 0);
    return ESP_FAIL;
}