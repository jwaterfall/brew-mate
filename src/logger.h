#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

enum class LogLevel {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3
};

class Logger {
private:
    static void log(LogLevel msgLevel, const char* prefix, const char* fmt, va_list args) {
        if (level >= msgLevel) {
            Serial.print(prefix);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            Serial.println(buffer);
        }
    }

public:
    static LogLevel level;
    
    static void setLevel(LogLevel lvl) {
        level = lvl;
    }
    
    static void error(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log(LogLevel::Error, "[ERROR] ", fmt, args);
        va_end(args);
    }
    
    static void warn(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log(LogLevel::Warn, "[WARN]  ", fmt, args);
        va_end(args);
    }
    
    static void info(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log(LogLevel::Info, "[INFO]  ", fmt, args);
        va_end(args);
    }
    
    static void debug(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log(LogLevel::Debug, "[DEBUG] ", fmt, args);
        va_end(args);
    }
};

LogLevel Logger::level = LogLevel::Info;

#endif
