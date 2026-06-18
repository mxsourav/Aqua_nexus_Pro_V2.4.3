# AquaNexus Pro - Smart Irrigation System

AquaNexus Pro is an automated planter and irrigation control system built around an ESP32 microcontroller. It features a local web-based control panel communicating via WebSockets, and links to the Blynk IoT Cloud for remote dashboard access.

The system monitors soil moisture levels and water tank depth, managing a water pump based on set thresholds.

## Features

- Real-time sensor telemetry and control toggles using WebSockets on the local network.
- Internet monitoring and status updates synced with Blynk IoT Cloud.
- Automatic irrigation based on configurable soil moisture thresholds.
- Premium dark-themed bento grid dashboard (HTML/CSS/JS).
- Local SPI TFT screen output.

## Hardware Configuration

- MCU: ESP32 (38-pin DevKit V1)
- Soil Moisture Sensor: Capacitive v1.2 (GPIO 34 / ADC1_CH6)
- Water Level Sensor: HC-SR04 Ultrasonic (Trigger: GPIO 33, Echo: GPIO 35)
- Pump Control: MOSFET (GPIO 25, LEDC PWM)
- Display: SPI TFT ST7735 (CS: GPIO 5, RST: GPIO 4, DC: GPIO 22, SDA: GPIO 23, SCL: GPIO 18, BL: GPIO 21)
- Alarm: Active Buzzer (GPIO 26)
- Input: Physical Mode Toggle Button (GPIO 27)

For full wiring specs, see pin_connection_details.md and component_roles.md.

## Project Structure

- include/config.h: Main configuration for pinouts, thresholds, and sensor limits.
- include/secrets.h: Local credentials (ignored by Git, copy from secrets.h.example).
- src/main.cpp: ESP32 setup and execution loop.
- index.html: Web control room client.
- documentation: md files containing detailed routing tables, component specifications, and schematics.

## Setup and Installation

### 1. Firmware Upload
1. Open this directory in VS Code with the PlatformIO extension installed.
2. Copy include/secrets.h.example to include/secrets.h.
3. Enter your Wi-Fi SSID, password, and Blynk Auth token in include/secrets.h.
4. Connect the ESP32 and click Upload in PlatformIO.

### 2. Web Dashboard
1. Open index.html directly in any web browser.
2. The page will auto-connect to the ESP32 WebSocket server using the default MDNS address (flura.local) or your ESP32's local IP address.

## Developer

Developed by MX SOURAV (https://github.com/mxsourav)
