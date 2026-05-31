// BrewMate host runner: reads raw sensor data from the scale over USB serial,
// runs the shared ScaleApp core, renders the OLED framebuffer (same Adafruit
// GFX code as the device) and streams it back for the device to blit. A
// periodic handshake puts the scale into proxy mode. Build/run via
// scripts/dev_host.sh. Linux-only (POSIX termios). Ctrl-C to quit.

#include <cstdio>
#include <cstring>
#include <string>
#include <chrono>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "scale_app.h"
#include "serial_protocol.h"
#include "display_render.h"

static int openSerial(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0) return -1;

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;  // 0.1s read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void handleLine(int fd, ScaleApp& app, GFXcanvas1& canvas, const std::string& line) {
    RawInputs in;
    if (!protocol::decodeInputs(line.c_str(), in)) {
        // Sensor lines we can't parse usually mean the firmware is older than
        // this host build (protocol changed) -- surface it instead of hanging.
        static bool warned = false;
        if (!warned && line[0] == 'I') {
            warned = true;
            fprintf(stderr, "\n[warning] can't parse the scale's data -- firmware is likely out of date.\n"
                            "Reflash:  pio run -e seeed_xiao_esp32c6 -t upload\n\n");
        }
        return;
    }

    ScaleOutputs out = app.tick(in);

    canvas.fillScreen(0);
    screen::renderMain(canvas, out.display);

    char frameLine[1100];
    int len = protocol::encodeFrame(canvas.getBuffer(), protocol::FRAME_BYTES,
                                    out.playBuzzer, frameLine, sizeof(frameLine));
    if (len > 0 && len < (int)sizeof(frameLine) - 1) {
        frameLine[len++] = '\n';
        if (write(fd, frameLine, len) < 0) { /* best effort */ }
    }

    printf("\rraw:%8ld  W:%8.2f g  flow:%6.2f g/s  timer:%s  bat:%3d%% (%.2fV)%s%s        ",
        (long)in.weightRaw, out.display.weight, out.display.flowRate, out.display.timerStr,
        out.battery.percent, out.battery.voltage,
        out.battery.charging ? " CHG" : "",
        out.display.tarePending ? "  [TARING]" : "");
    fflush(stdout);
}

int main(int argc, char** argv) {
    const char* port = (argc > 1) ? argv[1] : "/dev/ttyACM0";

    int fd = openSerial(port);
    if (fd < 0) {
        fprintf(stderr, "Failed to open serial port %s: %s\n", port, strerror(errno));
        fprintf(stderr, "Pass the port as an argument, e.g. ./host_runner /dev/ttyACM1\n");
        return 1;
    }

    fprintf(stderr, "BrewMate host runner connected to %s\n", port);
    fprintf(stderr, "The scale switches to proxy mode automatically. Ctrl-C to quit.\n\n");

    ScaleApp app;
    GFXcanvas1 canvas(128, 32);
    std::string lineBuf;
    char readBuf[256];

    // Periodic handshake so the scale flips into proxy mode (and stays there
    // until we stop, at which point it reverts to a normal standalone scale).
    using clock = std::chrono::steady_clock;
    auto lastBeacon = clock::now() - std::chrono::seconds(1);

    while (true) {
        auto nowT = clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(nowT - lastBeacon).count() >= 250) {
            const char* hello = "BMTAP\n";
            if (write(fd, hello, 6) < 0) { /* best effort */ }
            lastBeacon = nowT;
        }

        ssize_t n = read(fd, readBuf, sizeof(readBuf));
        if (n < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "\nserial read error: %s\n", strerror(errno));
            break;
        }
        for (ssize_t i = 0; i < n; i++) {
            char c = readBuf[i];
            if (c == '\n' || c == '\r') {
                if (!lineBuf.empty()) {
                    handleLine(fd, app, canvas, lineBuf);
                    lineBuf.clear();
                }
            } else if (lineBuf.size() < 250) {
                lineBuf.push_back(c);
            }
        }
    }

    close(fd);
    return 0;
}
