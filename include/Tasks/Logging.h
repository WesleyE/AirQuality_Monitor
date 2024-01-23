#pragma once

#include <HTTPClient.h>

#include <mutex>
#include <string>
#include <vector>

#include "Tasks/Task.h"
#include "common.h"
#include "helpers.h"

enum AQ_HttpLogLevel {
    ERROR,
    WARN,
    INFO,
    DEBUG,
    VERBOSE,
};

struct AQ_HttpLogConfig {
    String tag;
    AQ_HttpLogLevel level;
    String message;
};

class Logging : public Task {
   public:
    void run(void *data);
    void setup();

    static void log_info(const char *TAG, const char *format, ...);
    static void log_warning(const char *TAG, const char *format, ...);
    static void log_error(const char *TAG, const char *format, ...);

    static std::vector<AQ_HttpLogConfig> logs;
    static std::mutex mtx;

   private:
    HTTPClient httpClient;
};