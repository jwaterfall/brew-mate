# BrewMate

A compact coffee scale project for precise espresso brewing measurements.

## Description

BrewMate is a custom-built coffee scale designed for accurate weight and flow rate measurements during espresso extraction. The project features a compact form factor with a custom user interface optimized for the espresso workflow.

## Credits

While the code and 3D models for BrewMate were written from the ground up, this project is heavily inspired by the [WeighMyBru](https://github.com/031devstudios/weighmybru2) project. BrewMate uses the same electronics layout and similar components as weighmybru, and the code architecture was heavily inspired by their implementation.

The motivation for creating BrewMate was to build something smaller and less bulky than existing solutions, while developing custom user interfaces tailored to my preferences.

## Hardware

- ESP32-based microcontroller
  - XIAO ESP32C6
  - ESP32S3 SuperMini
- HX711 load cell amplifier
- SSD1306 OLED display (128x32)
- TTP223 touch sensors
- 500g load cell
- 3.7V 1000mAh battery

## Features

- Flow rate calculation
- Battery level monitoring with USB charging detection
- Compact form factor
- WiFi Access Point mode for initial setup
- WiFi client mode to connect to your home network
- Web-based user interface
- Real-time weight and battery status via web API

## WiFi Web Interface

The scale includes a WiFi Access Point mode for initial setup. When powered on, the scale creates a WiFi network named "BrewMate" (password: `brewmate123`) that allows you to configure the device and connect it to your home WiFi network.

### Setup

1. **Build the web UI:**

   ```bash
   cd web
   pnpm install
   pnpm build
   ```

2. **Upload web files to LittleFS:**

   ```bash
   pio run -t uploadfs
   ```

3. **Upload the firmware:**

   ```bash
   pio run -t upload
   ```

4. **Initial Setup:**

   - Connect your device to the "BrewMate" WiFi network
   - Open a browser and navigate to `http://192.168.4.1`
   - Configure the scale to connect to your home WiFi network
   - Once connected, the scale will switch to WiFi client mode and can be accessed via your local network

5. **Using the scale:**
   - After initial setup, access the web interface via your local network
   - The web interface displays real-time weight, battery status, and allows you to tare the scale

### API Endpoints

- `GET /api/status` - Returns current scale status (weight, battery percentage, voltage, USB connection status)
- `POST /api/tare` - Tares the scale

The display will show a WiFi AP icon when the access point is active (during initial setup). Once connected to your WiFi network, the device operates in client mode.

## License

This project is licensed under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) (Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International).

This means:

- **Attribution**: You must give appropriate credit
- **NonCommercial**: You may not use this work for commercial purposes
- **ShareAlike**: If you remix, transform, or build upon the material, you must distribute your contributions under the same license
