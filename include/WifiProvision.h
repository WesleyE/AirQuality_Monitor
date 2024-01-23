#pragma once

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_http_server.h>

class WifiProvision {
   public:
    WifiProvision();
    bool provisionWifi();
    bool forceReprovision();

   private:
    bool checkForSettings();
    bool connectToWifi();
    bool setupAP();

    void setupHttpServer();
    void setupDNSServer();

    String getWifiNetworks();

    IPAddress softapIP;
    IPAddress netMask;

    DNSServer dnsServer;
};
