#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/ledc.h>

class Buzzer {
private:
    uint8_t pin;
    bool initialized;
    
    static constexpr uint32_t DEFAULT_FREQUENCY = 2300;  // 2300 Hz
    static constexpr uint32_t DEFAULT_DURATION_MS = 45;  // 45 ms
    static constexpr uint8_t LEDC_RESOLUTION = 8;

public:
    Buzzer(uint8_t buzzerPin = 16) 
        : pin(buzzerPin), initialized(false) {
    }
    
    void begin() {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        
        // Set maximum drive capability for louder buzzer output
        gpio_set_drive_capability((gpio_num_t)pin, GPIO_DRIVE_CAP_3);
        
        // Setup LEDC for hardware PWM (interrupt-safe, clean tones)
        // ESP32-C6 uses ledcAttach which handles setup automatically
        ledcAttach(pin, DEFAULT_FREQUENCY, LEDC_RESOLUTION);
        
        initialized = true;
    }
    
    void play(uint32_t frequency = DEFAULT_FREQUENCY, uint32_t durationMs = DEFAULT_DURATION_MS) {
        if (!initialized) return;
        
        ledcWriteTone(pin, frequency);
        delay(durationMs);
        ledcWriteTone(pin, 0);
    }
    
    void playTouchSound() {
        play(DEFAULT_FREQUENCY, DEFAULT_DURATION_MS);
    }
    
    void stop() {
        if (initialized) {
            ledcWriteTone(pin, 0);
        }
    }
};

#endif

