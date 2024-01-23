# AirQuality V3

[<img src="./docs/banner.jpg" style="display: block; margin: auto; "/>](./docs/banner.jpg)

## What is the AirQuality V3?

The AirQuality V3 is an air quality sensor made from scratch. I wanted a cheaper sensor than the commercially available options, but more capabilities than the open-source options like [AirGradient](https://www.airgradient.com/).
It also gave me a good project to continue on my journey to learn more on developing electronics and embedded software.

The sensor periodically polls the sensors (as per the datasheets) and publishes the new values to a MQTT topic. Home Assistant is then able to pick up the values automatically trough auto-discovery.
OTA updates and configuration is supported trough a web interface.

This repository houses the source code, [BOM](./docs/AirQuality.csv), for the AirQuality V3 and the [KiCad files](./AirQuality_PCB_v3.zip).

### Sensors

- NOx and VOC gas sensing - [Sensirion SGP41 sensor](https://www.sensirion.com/sgp41)
- Temperature, Humidity and Pressure sensor - [Bosch BME280](https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/)
- CO2 - [SenseAir S8](https://senseair.com/products/size-counts/s8-residential/)
- Ambient Light [Vishay VEML7700](https://www.vishay.com/en/product/84286/)
- PM1.0, PM2.5 and PM10.0 particle concentration Particle [PMS5003](https://www.adafruit.com/product/3686)


## Building the code

1. Init the submodules: `git submodule update --init`
2. Run the PlatformIO Build `platformio run --environment debug`


## Libraries used

### Sensors
- adafruit/Adafruit BME280 Library _(BME280 Temperature, Humidity and Air Pressure sensor)_
- sensirion/Sensirion I2C SGP41 _(SGP41 Gas sensor)_
- adafruit/Adafruit VEML7700 Library _(VEML7700 Ambient Light Sensor)_
- fu-hsi/PMS Library _(PMS5003 particle sensor)_
- sensirion/Sensirion Gas Index Algorithm _(VOC and NOx gas index for Air Quality Levels)_
- jcomas/S8_UART _(SenseAir S8)_

### Others
- adafruit/Adafruit NeoPixel _(To drive the single NeoPixel communicating the current Air Quality level)_
- knolleary/PubSubClient _(MQTT Library for communication with Home Assistant)_
- bblanchon/ArduinoJson _(JSON Library for the HTTP server and MQTT)_
- [TheLartians/Observe](https://github.com/TheLartians/Observe) _(Simple thread-save events)_


## Datasheets

- [ESP32-S3 WROOM-1](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [BME280](https://www.mouser.com/datasheet/2/783/BST-BME280-DS002-1509607.pdf)
- [PMS5003](https://www.digikey.jp/htmldatasheets/production/2903006/0/0/1/pms5003-series-manual.html)
- [SenseAir S8](https://senseair.jp/wp-content/uploads/2019/06/S8-PWN-Product-specification-PSP122.pdf)
- [SGP-41](https://sensirion.com/media/documents/5FE8673C/61E96F50/Sensirion_Gas_Sensors_Datasheet_SGP41.pdf)
- [VEML7700](https://www.vishay.com/docs/84286/veml7700.pdf)


## Other info

- [Air Quality Index](https://airindex.eea.europa.eu/Map/AQI/Viewer/#)

```
Pollutant
(based on pollutant concentrations in µg/m3)
                                        Good	Fair	Moderate	Poor	Very poor	Extremely poor
Particles less than 2.5 µm (PM2.5)	    0-10	10-20	20-25	    25-50	50-75	    75-800
Particles less than 10 µm (PM10)	    0-20	20-40	40-50	    50-100	100-150	    150-1200


                                        Good	Fair	Moderate	Poor	    Very poor	Extremely poor
CO2 in ppm:
                                        0-350   350-800 800-1000    1000-1200   1200-1800   1800+
 https://www.co2meter.com/blogs/news/carbon-dioxide-indoor-levels-chart


NOx and VOC Index:
                                        Good	Fair	Moderate	Poor	    Very poor	Extremely poor
NOx Index:                              0-20    20-100  100-200     200-300     300-400     400+
VOC Index:                              0-150   150-200 200-250     250-300     300-400     400+
https://www.sensirion.com/media/documents/9F289B95/6294DFFC/Info_Note_NOx_Index.pdf
https://www.sensirion.com/media/documents/02232963/6294E043/Info_Note_VOC_Index.pdf
```