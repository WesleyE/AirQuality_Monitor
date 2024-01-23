#pragma once

#include <WebServer.h>
#include <WiFi.h>
#include <esp_http_server.h>

#include "Tasks/SensorRepository.h"
#include "Tasks/Task.h"

class HTTP : public Task {
   public:
    HTTP(SensorRepository *repository);
    void run(void *data);
    void setup();

    bool isInProvisioningMode;

   private:
    // WebServer server;
    SensorRepository *repository;

    httpd_handle_t server;

    const char *getPostData(httpd_req_t *req);
    String getWifiNetworks();

    esp_err_t handleIndex(httpd_req_t *req);
    esp_err_t handleProvisioningIndex(httpd_req_t *req);
    esp_err_t handleSensorValues(httpd_req_t *req);
    esp_err_t handleGetSettings(httpd_req_t *req);
    esp_err_t handleGetPreferences(httpd_req_t *req);
    esp_err_t handlePostPreferences(httpd_req_t *req);
    esp_err_t handlePostReset(httpd_req_t *req);
    esp_err_t handleGetNetworks(httpd_req_t *req);
    esp_err_t handlePostCalibrate(httpd_req_t *req);
    esp_err_t handleGetJavascript(httpd_req_t *req);
    esp_err_t handleGetStatus(httpd_req_t *req);
    esp_err_t handlePostOTA(httpd_req_t *req);
    esp_err_t handlePostResetOTA(httpd_req_t *req);

    esp_err_t handleOptions(httpd_req_t *req);
    esp_err_t httpErrorHandler(httpd_req_t *req, httpd_err_code_t err);
};