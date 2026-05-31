#pragma once

// Serial protocol between device and host dev runner. Newline-terminated lines:
//   "I,..."          device -> host : raw sensor reads (RawInputs)
//   "F,<buzz>,<hex>" host -> device : finished 128x32 framebuffer + buzzer flag
// Unparseable lines (e.g. stray logs) are ignored by both sides.

#include "scale_app.h"
#include <cstdio>
#include <cstring>

namespace protocol {

static const size_t FRAME_BYTES = 512;  // 128 x 32 / 8

inline int encodeInputs(const RawInputs& in, char* buf, size_t n) {
    return snprintf(buf, n, "I,%lu,%ld,%.4f,%u,%u,%.4f,%.4f,%d,%d,%d,%d,%d,%d,%d,%s",
        (unsigned long)in.nowMs, (long)in.weightRaw, in.weightCalFactor,
        in.batteryMv, in.vbusMv, in.batteryCalFactor, in.vbusCalFactor,
        in.batterySwitchRaw ? 1 : 0, in.tarePinRaw ? 1 : 0, in.powerPinRaw ? 1 : 0,
        in.wifiConnected ? 1 : 0, in.wifiApMode ? 1 : 0, in.bluetoothConnected ? 1 : 0,
        in.immediateTare ? 1 : 0, in.ipStr[0] ? in.ipStr : "-");
}

inline bool decodeInputs(const char* line, RawInputs& in) {
    if (line[0] != 'I') return false;
    unsigned long now;
    long weightRaw;
    float weightCal, battCal, vbusCal;
    unsigned battMv, vbusMv;
    int sw, tp, pp, wc, ap, bt, it;
    char ip[16] = {0};
    int matched = sscanf(line, "I,%lu,%ld,%f,%u,%u,%f,%f,%d,%d,%d,%d,%d,%d,%d,%15s",
        &now, &weightRaw, &weightCal, &battMv, &vbusMv, &battCal, &vbusCal,
        &sw, &tp, &pp, &wc, &ap, &bt, &it, ip);
    if (matched != 15) return false;
    in.nowMs = (uint32_t)now;
    in.weightRaw = (int32_t)weightRaw;
    in.weightCalFactor = weightCal;
    in.batteryMv = (uint16_t)battMv;
    in.vbusMv = (uint16_t)vbusMv;
    in.batteryCalFactor = battCal;
    in.vbusCalFactor = vbusCal;
    in.batterySwitchRaw = sw;
    in.tarePinRaw = tp;
    in.powerPinRaw = pp;
    in.wifiConnected = wc;
    in.wifiApMode = ap;
    in.bluetoothConnected = bt;
    in.immediateTare = it;
    if (ip[0] == '-' && ip[1] == '\0') ip[0] = '\0';
    strncpy(in.ipStr, ip, sizeof(in.ipStr));
    in.ipStr[sizeof(in.ipStr) - 1] = '\0';
    return true;
}

inline void toHex(const uint8_t* data, size_t len, char* out) {
    static const char* H = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = H[data[i] >> 4];
        out[i * 2 + 1] = H[data[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

inline int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

inline bool fromHex(const char* s, uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        int hi = hexVal(s[i * 2]);
        int lo = hexVal(s[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        data[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

inline int encodeFrame(const uint8_t* fb, size_t len, bool buzz, char* out, size_t n) {
    int p = snprintf(out, n, "F,%d,", buzz ? 1 : 0);
    if (p < 0 || (size_t)p + len * 2 + 1 > n) return -1;
    toHex(fb, len, out + p);
    return p + (int)(len * 2);
}

inline bool decodeFrame(const char* line, uint8_t* fb, size_t len, bool& buzz) {
    if (line[0] != 'F' || line[1] != ',') return false;
    const char* p = line + 2;
    if (*p != '0' && *p != '1') return false;
    buzz = (*p == '1');
    p++;
    if (*p != ',') return false;
    p++;
    if (strlen(p) < len * 2) return false;
    return fromHex(p, fb, len);
}

}  // namespace protocol
