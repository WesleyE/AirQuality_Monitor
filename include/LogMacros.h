#pragma once

#include "Tasks/Logging.h"

#undef ESP_LOGI
#define ESP_LOGI(tag, format, ...)                     \
    do {                                               \
        log_i("[%s] " format, tag, ##__VA_ARGS__);     \
        Logging::log_info(tag, format, ##__VA_ARGS__); \
    } while (0)

#undef ESP_LOGW
#define ESP_LOGW(tag, format, ...)                        \
    do {                                                  \
        log_w("[%s] " format, tag, ##__VA_ARGS__);        \
        Logging::log_warning(tag, format, ##__VA_ARGS__); \
    } while (0)

#undef ESP_LOGE
#define ESP_LOGE(tag, format, ...)                      \
    do {                                                \
        log_e("[%s] " format, tag, ##__VA_ARGS__);      \
        Logging::log_error(tag, format, ##__VA_ARGS__); \
    } while (0)
