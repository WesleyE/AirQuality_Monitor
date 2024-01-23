#pragma once

#include <string>
#include <vector>

#include "helpers.h"

struct LatestReadings;

/**
 * @brief The levels of air quality. Applicable to multiple types measurements (NOx, CO2, ...).
 *
 */
enum AirQualityLevel { GOOD, FAIR, MODERATE, POOR, VERY_POOR, EXTREMELY_POOR };

/**
 * @brief The AirQualityLevel's for all measurements that support levels.
 *
 */
struct AirQualityMeasurments {
    AirQualityLevel NOxLevel;
    AirQualityLevel VOCLevel;
    AirQualityLevel CO2Level;
    AirQualityLevel PM2_5Level;
    AirQualityLevel PM10Level;
    AirQualityLevel calculatedLevel;
};

class AirQualityLevels {
   public:
    AirQualityLevels();
    AirQualityMeasurments getLevels(const LatestReadings& readings);

    static std::string airQualityLevelToString(AirQualityLevel level);

   private:
    AirQualityLevel getAirQualityLevel(uint16_t measurement, const std::vector<std::pair<uint16_t, AirQualityLevel>>& levels);

    std::vector<std::pair<uint16_t, AirQualityLevel>> pm2_5Levels;
    std::vector<std::pair<uint16_t, AirQualityLevel>> pm10Levels;
    std::vector<std::pair<uint16_t, AirQualityLevel>> CO2Levels;
    std::vector<std::pair<uint16_t, AirQualityLevel>> NOxLevels;
    std::vector<std::pair<uint16_t, AirQualityLevel>> VOCLevels;
};