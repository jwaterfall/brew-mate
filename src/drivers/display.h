#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "board_config.h"
#include "scale_app.h"
#include "display_render.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

class Display {
private:
    Adafruit_SSD1306 display;
    bool initialized;

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
        display.clearDisplay();
        screen::renderBoot(display);
        display.display();
    }

    // Render a DisplayState locally (standalone mode).
    void showMainScreen(const DisplayState& s) {
        if (!initialized) return;
        display.clearDisplay();
        screen::renderMain(display, s);
        display.display();
    }

    // Blit a finished 128x32 framebuffer rendered on the host (proxy mode).
    void drawFrame(const uint8_t* buffer) {
        if (!initialized) return;
        display.clearDisplay();
        display.drawBitmap(0, 0, buffer, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        display.display();
    }
};
