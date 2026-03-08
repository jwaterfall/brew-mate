#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "board_config.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

class Display {
private:
    Adafruit_SSD1306 display;
    bool initialized;

    static const int WIFI_ICON_X = 0;
    static const int BLUETOOTH_ICON_X = 10;
    static const int BATTERY_PERCENT_X = 104;
    static const int BATTERY_PERCENT_Y = 2;
    static const int BATTERY_ICON_X = 106;
    static const int ICONS_Y = 0;
    
    static const int FLOW_RATE_X = 0;
    static const int FLOW_RATE_Y = 12;
    static const int FLOW_RATE_TEXT_SIZE = 1;
    static const int TIMER_X = 0;
    static const int TIMER_Y = 24;
    static const int TIMER_TEXT_SIZE = 1;
    static const int WEIGHT_X = 128;
    static const int WEIGHT_Y = 16;
    static const int WEIGHT_TEXT_SIZE = 2;
    
    static const int BATTERY_WIDTH = 20;
    static const int BATTERY_HEIGHT = 10;
    static const int BATTERY_BAR_COUNT = 3;
    static const int BATTERY_BAR_WIDTH = 3;
    static const int BATTERY_BAR_SPACING = 2;
    static const int BATTERY_BAR_HEIGHT = 6;
    
    static const unsigned char logoBitmap[];

    void clear() {
        if (initialized) {
            display.clearDisplay();
        }
    }

    void show() {
        if (initialized) {
            display.display();
        }
    }

    void drawText(int x, int y, int size, const char* text, int color = 1, bool rightAlign = false) {
        if (!initialized) return;
        
        display.setTextSize(size);
        display.setTextColor(color);
        
        if (rightAlign) {
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
            display.setCursor(x - w, y);
        } else {
            display.setCursor(x, y);
        }
        
        display.print(text);
    }

    void drawText(int x, int y, int size, float value, int decimals, int color = 1, const char* suffix = nullptr, bool rightAlign = false) {
        if (!initialized) return;
        
        display.setTextSize(size);
        display.setTextColor(color);
        
        if (rightAlign) {
            char buffer[16];
            int len = snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
            if (suffix != nullptr) {
                snprintf(buffer + len, sizeof(buffer) - len, "%s", suffix);
            }
            
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);
            display.setCursor(x - w, y);
            display.print(buffer);
        } else {
            display.setCursor(x, y);
            display.print(value, decimals);
            if (suffix != nullptr) {
                display.print(suffix);
            }
        }
    }

    void drawWifiIcon() {
        if (!initialized) return;
        
        display.drawPixel(WIFI_ICON_X + 3, ICONS_Y + 7, 1);
        display.drawLine(WIFI_ICON_X + 2, ICONS_Y + 5, WIFI_ICON_X + 4, ICONS_Y + 5, 1);
        display.drawLine(WIFI_ICON_X + 1, ICONS_Y + 3, WIFI_ICON_X + 5, ICONS_Y + 3, 1);
        display.drawLine(WIFI_ICON_X, ICONS_Y + 1, WIFI_ICON_X + 6, ICONS_Y + 1, 1);
    }
    
    void drawWifiApIcon() {
        if (!initialized) return;
        
        display.drawRect(WIFI_ICON_X + 1, ICONS_Y + 3, 5, 4, 1);
        display.drawLine(WIFI_ICON_X + 2, ICONS_Y + 1, WIFI_ICON_X + 2, ICONS_Y + 3, 1);
        display.drawLine(WIFI_ICON_X + 5, ICONS_Y + 1, WIFI_ICON_X + 5, ICONS_Y + 3, 1);
        display.drawPixel(WIFI_ICON_X + 2, ICONS_Y + 4, 1);
        display.drawPixel(WIFI_ICON_X + 5, ICONS_Y + 5, 1);
    }
    
    void drawBluetoothIcon() {
        if (!initialized) return;
        
        display.drawLine(BLUETOOTH_ICON_X + 3, ICONS_Y, BLUETOOTH_ICON_X + 3, ICONS_Y + 7, 1);
        display.drawLine(BLUETOOTH_ICON_X + 1, ICONS_Y + 2, BLUETOOTH_ICON_X + 3, ICONS_Y + 4, 1);
        display.drawLine(BLUETOOTH_ICON_X + 3, ICONS_Y + 4, BLUETOOTH_ICON_X + 5, ICONS_Y + 2, 1);
        display.drawLine(BLUETOOTH_ICON_X + 1, ICONS_Y + 5, BLUETOOTH_ICON_X + 3, ICONS_Y + 3, 1);
        display.drawLine(BLUETOOTH_ICON_X + 3, ICONS_Y + 3, BLUETOOTH_ICON_X + 5, ICONS_Y + 5, 1);
    }
    
    void drawBatteryIcon(uint8_t barCount, bool isCharging = false, int animationFrame = 0, bool disconnected = false) {
        if (!initialized) return;
        
        if (disconnected && animationFrame >= 2) {
            return;
        }
        
        display.drawRect(BATTERY_ICON_X, ICONS_Y, BATTERY_WIDTH, BATTERY_HEIGHT, 1);
        display.fillRect(BATTERY_ICON_X + BATTERY_WIDTH, ICONS_Y + 2, 2, 6, 1);
        
        if (disconnected) {
            display.drawLine(BATTERY_ICON_X + 1, ICONS_Y + 1, BATTERY_ICON_X + BATTERY_WIDTH - 2, ICONS_Y + BATTERY_HEIGHT - 2, 1);
            display.drawLine(BATTERY_ICON_X + 1, ICONS_Y + BATTERY_HEIGHT - 2, BATTERY_ICON_X + BATTERY_WIDTH - 2, ICONS_Y + 1, 1);
            return;
        }
        
        static const int START_X = BATTERY_ICON_X + 3;
        static const int START_Y = ICONS_Y + 2;
        
        int barsToShow = isCharging ? animationFrame : barCount;
        
        for (int i = 0; i < BATTERY_BAR_COUNT; i++) {
            int barX = START_X + i * (BATTERY_BAR_WIDTH + BATTERY_BAR_SPACING);
            display.fillRect(barX, START_Y, BATTERY_BAR_WIDTH, BATTERY_BAR_HEIGHT, (i < barsToShow) ? 1 : 0);
        }
    }

public:
    Display() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), initialized(false) {}
    
    bool begin() {
        Wire.begin(SDA_PIN, SCL_PIN);
        delay(500);
        
        static const uint8_t addresses[] = {OLED_ADDRESS, 0x3D};
        for (uint8_t addr : addresses) {
            if (display.begin(SSD1306_SWITCHCAPVCC, addr)) {
                initialized = true;
                return true;
            }
        }
        return false;
    }
    
    void showBootScreen() {
        if (!initialized) return;
        
        clear();
        display.drawBitmap(2, 6, logoBitmap, 16, 16, 1);
        drawText(26, 10, 2, "BrewMate");
        show();
    }
    
    void showMainScreen(uint8_t batteryBarCount, uint8_t batteryPercent, float weight, const char* timer, float flowRate, bool isCharging = false, int animationFrame = 0, bool batteryDisconnected = false, bool wifiConnected = false, bool wifiApMode = false, bool bluetoothConnected = false, bool tarePending = false) {
        if (!initialized) return;
        
        clear();
        
        drawText(FLOW_RATE_X, FLOW_RATE_Y, FLOW_RATE_TEXT_SIZE, flowRate, 1, 1, "g/s");
        
        if (wifiConnected) {
            if (wifiApMode) {
                drawWifiApIcon();
            } else {
                drawWifiIcon();
            }
        }
        if (bluetoothConnected) drawBluetoothIcon();
        
        if (!batteryDisconnected) {
            char percentStr[5];
            snprintf(percentStr, sizeof(percentStr), "%d%%", batteryPercent);
            drawText(BATTERY_PERCENT_X, BATTERY_PERCENT_Y, 1, percentStr, 1, true);
        }
        drawBatteryIcon(batteryBarCount, isCharging, animationFrame, batteryDisconnected);
        
        drawText(TIMER_X, TIMER_Y, TIMER_TEXT_SIZE, timer);
        
        if (!tarePending || (animationFrame % 2 == 0)) {
            float displayWeight = weight;
            if (displayWeight < 0.0f && displayWeight > -0.1f) {
                displayWeight = 0.0f;
            }
            drawText(WEIGHT_X, WEIGHT_Y, WEIGHT_TEXT_SIZE, displayWeight, 1, 1, "g", true);
        }
        
        show();
    }
    
    bool isInitialized() {
        return initialized;
    }
};

const unsigned char Display::logoBitmap[] = {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x08, 0x20,
    0x04, 0x40,
    0x08, 0x20,
    0x00, 0x00,
    0x7F, 0xFC,
    0x40, 0x04,
    0x40, 0x04,
    0x40, 0x04,
    0x40, 0x04,
    0x40, 0x04,
    0x7F, 0xFC,
    0x3F, 0xF8,
    0x1F, 0xF0
};

#endif
