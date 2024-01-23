#pragma once

#include <string.h>

#include "common.h"
#include "time.h"

String getSerialNumber();
String getValue(String data, char separator, int index);
void getRandomStr(char* output, int len);
time_t getUnixTime();
int64_t getTimeSinceBootInSeconds();

#ifndef MIN
    #define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
    #define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

bool getShouldLogValues();