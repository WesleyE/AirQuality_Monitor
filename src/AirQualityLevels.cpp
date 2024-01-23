#include "AirQualityLevels.h"

#include "Tasks/SensorRepository.h"

AirQualityLevels::AirQualityLevels() {
    // See the readme.md
    pm2_5Levels = {{0.0, AirQualityLevel::GOOD},  {10.0, AirQualityLevel::FAIR},      {20.0, AirQualityLevel::MODERATE},
                   {25.0, AirQualityLevel::POOR}, {50.0, AirQualityLevel::VERY_POOR}, {75.0, AirQualityLevel::EXTREMELY_POOR}};

    pm10Levels = {{0.0, AirQualityLevel::GOOD},  {20.0, AirQualityLevel::FAIR},       {40.0, AirQualityLevel::MODERATE},
                  {50.0, AirQualityLevel::POOR}, {100.0, AirQualityLevel::VERY_POOR}, {150.0, AirQualityLevel::EXTREMELY_POOR}};

    CO2Levels = {{0.0, AirQualityLevel::GOOD},    {500.0, AirQualityLevel::FAIR},       {800.0, AirQualityLevel::MODERATE},
                 {1000.0, AirQualityLevel::POOR}, {1200.0, AirQualityLevel::VERY_POOR}, {1800.0, AirQualityLevel::EXTREMELY_POOR}};

    NOxLevels = {{0.0, AirQualityLevel::GOOD},   {20.0, AirQualityLevel::FAIR},       {100.0, AirQualityLevel::MODERATE},
                 {200.0, AirQualityLevel::POOR}, {300.0, AirQualityLevel::VERY_POOR}, {400.0, AirQualityLevel::EXTREMELY_POOR}};

    VOCLevels = {{0.0, AirQualityLevel::GOOD},   {150.0, AirQualityLevel::FAIR},      {200.0, AirQualityLevel::MODERATE},
                 {250.0, AirQualityLevel::POOR}, {300.0, AirQualityLevel::VERY_POOR}, {400.0, AirQualityLevel::EXTREMELY_POOR}};
}

// Function to get the air quality level based on the measurement value and predefined level ranges
AirQualityLevel AirQualityLevels::getAirQualityLevel(uint16_t measurement, const std::vector<std::pair<uint16_t, AirQualityLevel>>& levels) {
    for (auto it = levels.rbegin(); it != levels.rend(); ++it) {
        if (measurement >= it->first) {
            return it->second;
        }
    }
    return levels.front().second;
}

AirQualityMeasurments AirQualityLevels::getLevels(const LatestReadings& readings) {
    AirQualityMeasurments aqm;

    aqm.PM2_5Level = getAirQualityLevel(readings.latestPMS5003Reading.Atmospheric_PM_2_5, pm2_5Levels);
    aqm.PM10Level = getAirQualityLevel(readings.latestPMS5003Reading.Atmospheric_PM_10_0, pm10Levels);

    aqm.CO2Level = getAirQualityLevel(readings.latestSenseAirS8Reading.co2, CO2Levels);

    aqm.NOxLevel = getAirQualityLevel(readings.latestSGP41Reading.noxIndex, NOxLevels);
    aqm.VOCLevel = getAirQualityLevel(readings.latestSGP41Reading.vocIndex, VOCLevels);

    aqm.calculatedLevel = MAX(MAX(MAX(MAX(aqm.PM2_5Level, aqm.PM10Level), aqm.CO2Level), aqm.NOxLevel), aqm.VOCLevel);

    return aqm;
}

std::string AirQualityLevels::airQualityLevelToString(AirQualityLevel level) {
    switch (level) {
        case AirQualityLevel::GOOD:
            return "Good";
        case AirQualityLevel::FAIR:
            return "Fair";
        case AirQualityLevel::MODERATE:
            return "Moderate";
        case AirQualityLevel::POOR:
            return "Poor";
        case AirQualityLevel::VERY_POOR:
            return "Very poor";
        case AirQualityLevel::EXTREMELY_POOR:
            return "Extremely poor";
        default:
            return "Unknown";
    }
}