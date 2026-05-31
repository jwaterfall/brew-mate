#ifndef DISPLAY_RENDER_H
#define DISPLAY_RENDER_H

// Shared screen rendering, drawn into any Adafruit_GFX surface: the real
// SSD1306 on-device and a GFXcanvas1 on the host, so the OLED is pixel-identical
// either way. The caller clears and flushes; these functions only draw.

#include <Adafruit_GFX.h>
#include "scale_app.h"

namespace screen {

namespace layout {
constexpr int WIFI_ICON_X = 0;
constexpr int BLUETOOTH_ICON_X = 10;
constexpr int BATTERY_PERCENT_X = 104;
constexpr int BATTERY_PERCENT_Y = 2;
constexpr int BATTERY_ICON_X = 106;
constexpr int ICONS_Y = 0;

constexpr int FLOW_RATE_X = 0;
constexpr int FLOW_RATE_Y = 12;
constexpr int FLOW_RATE_TEXT_SIZE = 1;
constexpr int TIMER_X = 0;
constexpr int TIMER_Y = 24;
constexpr int TIMER_TEXT_SIZE = 1;
constexpr int WEIGHT_X = 128;
constexpr int WEIGHT_Y = 16;
constexpr int WEIGHT_TEXT_SIZE = 2;

constexpr int BATTERY_WIDTH = 20;
constexpr int BATTERY_HEIGHT = 10;
constexpr int BATTERY_BAR_COUNT = 3;
constexpr int BATTERY_BAR_WIDTH = 3;
constexpr int BATTERY_BAR_SPACING = 2;
constexpr int BATTERY_BAR_HEIGHT = 6;
}  // namespace layout

inline void drawText(Adafruit_GFX& gfx, int x, int y, int size, const char* text, int color = 1, bool rightAlign = false) {
    gfx.setTextSize(size);
    gfx.setTextColor(color);
    if (rightAlign) {
        int16_t x1, y1;
        uint16_t w, h;
        gfx.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        gfx.setCursor(x - w, y);
    } else {
        gfx.setCursor(x, y);
    }
    gfx.print(text);
}

inline void drawText(Adafruit_GFX& gfx, int x, int y, int size, float value, int decimals, int color = 1, const char* suffix = nullptr, bool rightAlign = false) {
    gfx.setTextSize(size);
    gfx.setTextColor(color);
    if (rightAlign) {
        char buffer[16];
        int len = snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
        if (suffix != nullptr) {
            snprintf(buffer + len, sizeof(buffer) - len, "%s", suffix);
        }
        int16_t x1, y1;
        uint16_t w, h;
        gfx.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);
        gfx.setCursor(x - w, y);
        gfx.print(buffer);
    } else {
        gfx.setCursor(x, y);
        gfx.print(value, decimals);
        if (suffix != nullptr) {
            gfx.print(suffix);
        }
    }
}

inline void drawWifiIcon(Adafruit_GFX& gfx) {
    using namespace layout;
    gfx.drawPixel(WIFI_ICON_X + 3, ICONS_Y + 7, 1);
    gfx.drawLine(WIFI_ICON_X + 2, ICONS_Y + 5, WIFI_ICON_X + 4, ICONS_Y + 5, 1);
    gfx.drawLine(WIFI_ICON_X + 1, ICONS_Y + 3, WIFI_ICON_X + 5, ICONS_Y + 3, 1);
    gfx.drawLine(WIFI_ICON_X, ICONS_Y + 1, WIFI_ICON_X + 6, ICONS_Y + 1, 1);
}

inline void drawWifiApIcon(Adafruit_GFX& gfx) {
    using namespace layout;
    gfx.drawRect(WIFI_ICON_X + 1, ICONS_Y + 3, 5, 4, 1);
    gfx.drawLine(WIFI_ICON_X + 2, ICONS_Y + 1, WIFI_ICON_X + 2, ICONS_Y + 3, 1);
    gfx.drawLine(WIFI_ICON_X + 5, ICONS_Y + 1, WIFI_ICON_X + 5, ICONS_Y + 3, 1);
    gfx.drawPixel(WIFI_ICON_X + 2, ICONS_Y + 4, 1);
    gfx.drawPixel(WIFI_ICON_X + 5, ICONS_Y + 5, 1);
}

inline void drawBluetoothIcon(Adafruit_GFX& gfx) {
    using namespace layout;
    gfx.drawLine(BLUETOOTH_ICON_X + 3, ICONS_Y, BLUETOOTH_ICON_X + 3, ICONS_Y + 7, 1);
    gfx.drawLine(BLUETOOTH_ICON_X + 1, ICONS_Y + 2, BLUETOOTH_ICON_X + 3, ICONS_Y + 4, 1);
    gfx.drawLine(BLUETOOTH_ICON_X + 3, ICONS_Y + 4, BLUETOOTH_ICON_X + 5, ICONS_Y + 2, 1);
    gfx.drawLine(BLUETOOTH_ICON_X + 1, ICONS_Y + 5, BLUETOOTH_ICON_X + 3, ICONS_Y + 3, 1);
    gfx.drawLine(BLUETOOTH_ICON_X + 3, ICONS_Y + 3, BLUETOOTH_ICON_X + 5, ICONS_Y + 5, 1);
}

inline void drawBatteryIcon(Adafruit_GFX& gfx, uint8_t barCount, bool isCharging, int animationFrame, bool disconnected) {
    using namespace layout;
    if (disconnected && animationFrame >= 2) {
        return;
    }

    gfx.drawRect(BATTERY_ICON_X, ICONS_Y, BATTERY_WIDTH, BATTERY_HEIGHT, 1);
    gfx.fillRect(BATTERY_ICON_X + BATTERY_WIDTH, ICONS_Y + 2, 2, 6, 1);

    if (disconnected) {
        gfx.drawLine(BATTERY_ICON_X + 1, ICONS_Y + 1, BATTERY_ICON_X + BATTERY_WIDTH - 2, ICONS_Y + BATTERY_HEIGHT - 2, 1);
        gfx.drawLine(BATTERY_ICON_X + 1, ICONS_Y + BATTERY_HEIGHT - 2, BATTERY_ICON_X + BATTERY_WIDTH - 2, ICONS_Y + 1, 1);
        return;
    }

    const int START_X = BATTERY_ICON_X + 3;
    const int START_Y = ICONS_Y + 2;
    int barsToShow = isCharging ? animationFrame : barCount;

    for (int i = 0; i < BATTERY_BAR_COUNT; i++) {
        int barX = START_X + i * (BATTERY_BAR_WIDTH + BATTERY_BAR_SPACING);
        gfx.fillRect(barX, START_Y, BATTERY_BAR_WIDTH, BATTERY_BAR_HEIGHT, (i < barsToShow) ? 1 : 0);
    }
}

inline void renderInfo(Adafruit_GFX& gfx, const DisplayState& s) {
    drawText(gfx, 0, 3, 1, s.wifiApMode ? "AP: BrewMate" : "Connect to:");
    drawText(gfx, 0, 18, 1, s.ipStr);
}

inline void renderMain(Adafruit_GFX& gfx, const DisplayState& s) {
    using namespace layout;

    if (s.showInfo) {
        renderInfo(gfx, s);
        return;
    }

    drawText(gfx, FLOW_RATE_X, FLOW_RATE_Y, FLOW_RATE_TEXT_SIZE, s.flowRate, 1, 1, "g/s");

    if (s.wifiConnected) {
        if (s.wifiApMode) drawWifiApIcon(gfx);
        else drawWifiIcon(gfx);
    }
    if (s.bluetoothConnected) drawBluetoothIcon(gfx);

    if (!s.batteryDisconnected) {
        char percentStr[5];
        snprintf(percentStr, sizeof(percentStr), "%d%%", s.batteryPercent);
        drawText(gfx, BATTERY_PERCENT_X, BATTERY_PERCENT_Y, 1, percentStr, 1, true);
    }
    drawBatteryIcon(gfx, s.batteryBarCount, s.isCharging, s.animationFrame, s.batteryDisconnected);

    drawText(gfx, TIMER_X, TIMER_Y, TIMER_TEXT_SIZE, s.timerStr);

    if (!s.tarePending || (s.animationFrame % 2 == 0)) {
        float displayWeight = s.weight;
        if (displayWeight < 0.0f && displayWeight > -0.1f) {
            displayWeight = 0.0f;
        }
        drawText(gfx, WEIGHT_X, WEIGHT_Y, WEIGHT_TEXT_SIZE, displayWeight, 1, 1, "g", true);
    }
}

inline void renderBoot(Adafruit_GFX& gfx) {
    static const unsigned char logoBitmap[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x20,
        0x04, 0x40, 0x08, 0x20, 0x00, 0x00, 0x7F, 0xFC,
        0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04,
        0x40, 0x04, 0x7F, 0xFC, 0x3F, 0xF8, 0x1F, 0xF0
    };
    gfx.drawBitmap(2, 6, logoBitmap, 16, 16, 1);
    drawText(gfx, 26, 10, 2, "BrewMate");
}

}  // namespace screen

#endif
