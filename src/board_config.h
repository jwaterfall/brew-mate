#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#ifdef BOARD_XIAO_ESP32C6
    #define SDA_PIN 20
    #define SCL_PIN 18
    #define BATTERY_PIN 0
    #define VBUS_PIN 2
    #define BATTERY_SWITCH_PIN 1
    #define HX711_DOUT 22
    #define HX711_SCK 23
    #define TARE_PIN 21
    #define POWER_PIN 19

#elif defined(BOARD_ESP32S3_SUPERMINI)
    #define SDA_PIN 8
    #define SCL_PIN 9
    #define BATTERY_PIN 7
    #define VBUS_PIN 10
    #define BATTERY_SWITCH_PIN 1
    #define HX711_DOUT 5
    #define HX711_SCK 6
    #define TARE_PIN 4
    #define POWER_PIN 3

#else
    #error "Unknown board. Please define BOARD_XIAO_ESP32C6 or BOARD_ESP32S3_SUPERMINI in platformio.ini"
#endif

#endif

