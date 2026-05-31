#ifndef HOST_COMPAT_PRINT_H
#define HOST_COMPAT_PRINT_H

// Minimal Arduino Print base for host builds: Adafruit_GFX inherits it and
// overrides write(uint8_t); our rendering uses print(const char*) and
// print(double, int).

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            if (write(*buffer++)) n++;
            else break;
        }
        return n;
    }
    size_t write(const char* s) { return s ? write((const uint8_t*)s, strlen(s)) : 0; }
    size_t print(const char* s) { return write(s); }
    size_t print(double n, int digits = 2) { char b[40]; return write((const uint8_t*)b, snprintf(b, sizeof(b), "%.*f", digits, n)); }
};

#endif
