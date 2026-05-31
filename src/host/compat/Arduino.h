#ifndef HOST_COMPAT_ARDUINO_H
#define HOST_COMPAT_ARDUINO_H

// Minimal Arduino shim so Adafruit_GFX (and our shared rendering code) compile
// on the host. Only what the GFX core actually touches is provided.

#ifndef ARDUINO
#define ARDUINO 10805
#endif

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include "Print.h"

typedef uint8_t boolean;
typedef uint8_t byte;

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PGM_P
#define PGM_P const char*
#endif

// Flash reads are plain memory reads on the host.
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#endif
#ifndef pgm_read_pointer
#define pgm_read_pointer(addr) (*(void* const*)(addr))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

class __FlashStringHelper;
#ifndef F
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper*>(string_literal))
#endif

// Just the slice of Arduino String that Adafruit_GFX::getTextBounds(String) uses.
class String {
public:
    String(const char* p = "") : _s(p ? p : "") {}
    size_t length() const { return strlen(_s); }
    const char* c_str() const { return _s; }
private:
    const char* _s;
};

inline void yield() {}

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.295779513082320876798154814105
#endif
#ifndef radians
#define radians(deg) ((deg) * DEG_TO_RAD)
#endif
#ifndef degrees
#define degrees(rad) ((rad) * RAD_TO_DEG)
#endif

#endif
