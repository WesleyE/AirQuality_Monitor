#include "Tasks/Logging.h"

#include <ArduinoJson.h>
#include <Preferences.h>

#include "common.h"
#include "events.h"

#define LOG_BUFFER_SIZE 256
#define LOG_JSON_SIZE 1536  // Calculated for 5 logs max

#define LOG_SEND_INTERVAL 30
#define MAX_MSG_QUEUE_SIZE 15  // Max messages in the queue to be send

static const char *TAG = "LOG";

std::mutex Logging::mtx;
std::vector<AQ_HttpLogConfig> Logging::logs;

void Logging::run(void *data) {
    ESP_LOGI(TAG, "Task Logging started");

    setup();

    logEvent.connect([&](AQ_HttpLogConfig config) {
        this->mtx.lock();
        // Skip logging when the queue is bigger than MAX_MSG_QUEUE_SIZE messages
        if (logs.size() == MAX_MSG_QUEUE_SIZE - 1) {
            ESP_LOGE(TAG, "Dropping next messages since the queue is larger than %d", MAX_MSG_QUEUE_SIZE);
        }
        logs.push_back(config);
        this->mtx.unlock();
    });

    int64_t lastPushTime = 0;
    for (;;) {
        vTaskDelay(250 / portTICK_RATE_MS);

        this->mtx.lock();

        if (this->logs.size() == 0) {
            this->mtx.unlock();
            continue;
        }

        // Send logs if larger than 5
        bool shouldSendLogs = this->logs.size() >= 5;
        for (auto &log : this->logs) {
            if (log.level == AQ_HttpLogLevel::ERROR || log.level == AQ_HttpLogLevel::WARN) {
                shouldSendLogs = true;
                break;
            }
        }
        this->mtx.unlock();

        int64_t currentTime = getTimeSinceBootInSeconds();
        if (currentTime > lastPushTime + LOG_SEND_INTERVAL) {
            shouldSendLogs = true;
        }

        if (!shouldSendLogs) {
            continue;
        }

        this->mtx.lock();
        StaticJsonDocument<LOG_JSON_SIZE> doc;
        int messagesScheduled = 0;
        for (auto &log : this->logs) {
            // Max 5 messages in one post
            if (messagesScheduled == 5) {
                break;
            }

            JsonObject doc_0 = doc.createNestedObject();
            doc_0["log"] = log.message;
            switch (log.level) {
                case AQ_HttpLogLevel::ERROR:
                    doc_0["level"] = "ERROR";
                    break;
                case AQ_HttpLogLevel::WARN:
                    doc_0["level"] = "WARN";
                    break;
                case AQ_HttpLogLevel::INFO:
                    doc_0["level"] = "INFO";
                    break;
                case AQ_HttpLogLevel::DEBUG:
                    doc_0["level"] = "DEBUG";
                    break;
                case AQ_HttpLogLevel::VERBOSE:
                    doc_0["level"] = "VERBOSE";
                    break;
                default:
                    doc_0["level"] = "INFO";
                    break;
            }
            doc_0["tag"] = log.tag;
            doc_0["device_id"] = getSerialNumber();
            doc_0["device_type"] = "airquality";

            messagesScheduled++;
        }

        String jsonOutput = "";
        serializeJson(doc, jsonOutput);

        int resultCode = httpClient.POST(jsonOutput);
        if (resultCode == 200) {
            lastPushTime = currentTime;
            this->logs.clear();
            this->mtx.unlock();
        } else {
            ESP_LOGE(TAG, "Could not send logs to server: %d", resultCode);
            this->mtx.unlock();
        }
    }
}

void Logging::log_info(const char *TAG, const char *format, ...) {
    char buffer[LOG_BUFFER_SIZE];

    // Log normally
    va_list args;
    va_start(args, format);
    int requiredSize = vsnprintf(nullptr, 0, format, args) + 1;  // +1 for the null terminator
    if (requiredSize <= LOG_BUFFER_SIZE) {
        vsprintf(buffer, format, args);
    } else {
        // Truncate the message if the message is too large to fit the buffer
        strncpy(buffer, format, LOG_BUFFER_SIZE - 1);
        buffer[LOG_BUFFER_SIZE - 1] = '\0';
    }
    va_end(args);

    // Send to list to be transfered
    logEvent.emit(AQ_HttpLogConfig{.tag = TAG, .level = AQ_HttpLogLevel::INFO, .message = buffer});
}

void Logging::log_warning(const char *TAG, const char *format, ...) {
    char buffer[LOG_BUFFER_SIZE];

    // Log normally
    va_list args;
    va_start(args, format);
    int requiredSize = vsnprintf(nullptr, 0, format, args) + 1;  // +1 for the null terminator
    if (requiredSize <= LOG_BUFFER_SIZE) {
        vsprintf(buffer, format, args);
    } else {
        // Truncate the message if the message is too large to fit the buffer
        strncpy(buffer, format, LOG_BUFFER_SIZE - 1);
        buffer[LOG_BUFFER_SIZE - 1] = '\0';
    }
    va_end(args);

    // Send to list to be transfered
    logEvent.emit(AQ_HttpLogConfig{.tag = TAG, .level = AQ_HttpLogLevel::WARN, .message = buffer});
}

void Logging::log_error(const char *TAG, const char *format, ...) {
    char buffer[LOG_BUFFER_SIZE];

    // Log normally
    va_list args;
    va_start(args, format);
    int requiredSize = vsnprintf(nullptr, 0, format, args) + 1;  // +1 for the null terminator
    if (requiredSize <= LOG_BUFFER_SIZE) {
        vsprintf(buffer, format, args);
    } else {
        // Truncate the message if the message is too large to fit the buffer
        strncpy(buffer, format, LOG_BUFFER_SIZE - 1);
        buffer[LOG_BUFFER_SIZE - 1] = '\0';
    }
    va_end(args);

    // Send to list to be transfered
    logEvent.emit(AQ_HttpLogConfig{.tag = TAG, .level = AQ_HttpLogLevel::ERROR, .message = buffer});
}

void Logging::setup() {
    ESP_LOGI(TAG, "Setting up logging");

    Preferences preferences;
    preferences.begin("preferences", true);

    if (preferences.getString("logHost") == "") {
        ESP_LOGE(TAG, "No host set, not continuing logging.");
        for (;;) {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }

    httpClient.begin("http://" + preferences.getString("logHost") + "/api/default/airquality/_json");
    httpClient.setAuthorization(preferences.getString("logUsername").c_str(), preferences.getString("logPassword").c_str());
    httpClient.setReuse(false); // The server disconnects quite often, so keep-alive should be turned off in this instance.

    preferences.end();
}