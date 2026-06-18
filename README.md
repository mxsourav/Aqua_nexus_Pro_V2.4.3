# AquaNexus Pro — Smart Irrigation System

AquaNexus Pro is a state-of-the-art automated planter and irrigation control system powered by the **ESP32 DevKit V1** microcontroller, a responsive local WebSocket web client, and the **Blynk IoT Cloud** platform.

---

## 🌟 Key Features

* **Real-time Local Control**: Uses a bi-directional persistent WebSocket connection to control pump overrides, monitor sensors, and change configurations with sub-second latency.
* **Cloud Integration**: Synchronizes telemetry and settings with the Blynk IoT Cloud for global remote monitoring and alerting.
* **Interactive 3D Bento Control Room**: A modern, premium dark-themed web dashboard with responsive grids, interactive 3D tactile buttons, custom scrollbars, and animated reservoir fill-levels.
* **Dual-sensor Fallbacks**: Utilizes both a Capacitive Soil Moisture Sensor (v1.2) for ground level moisture and an HC-SR04 Ultrasonic Sensor for water reservoir depth tracking.
* **OLED/TFT Display Interface**: Renders real-time statuses and local stats directly on an attached SPI TFT screen.

---

## 🛠️ Hardware Component Map & Pinouts

For detailed specifications, see [pin_connection_details.md](file:///F:/Work%20for%20Ankita/Aqua_nexus_Pro_V2.4.3/pin_connection_details.md) and [component_roles.md](file:///F:/Work%20for%20Ankita/Aqua_nexus_Pro_V2.4.3/component_roles.md).

| Component | ESP32 GPIO | Description / Mode |
| --- | --- | --- |
| **TFT CS** | GPIO 5 | SPI Chip Select |
| **TFT RST** | GPIO 4 | Display Reset |
| **TFT DC** | GPIO 22 | Data / Command |
| **TFT MOSI** | GPIO 23 | SPI MOSI (SDA) |
| **TFT SCK** | GPIO 18 | SPI SCK (SCL) |
| **TFT BL** | GPIO 21 | Backlight Enable |
| **HC-SR04 Trigger** | GPIO 33 | Ultrasonic Trigger Output |
| **HC-SR04 Echo** | GPIO 35 | Ultrasonic Echo Input |
| **Moisture Sensor** | GPIO 34 | Analog Input (ADC1 CH6) |
| **MOSFET Gate** | GPIO 25 | PWM Duty Cycle output for Pump |
| **Buzzer** | GPIO 26 | Audible Alert Output |
| **Push Button** | GPIO 27 | Manual Display toggle button |
| **Float Switch** | GPIO 13 | Backup Tank Empty Input |

---

## ⚙️ Firmware Configuration & Setup

The firmware is developed using **PlatformIO** (VS Code).

1. Open this directory in **VS Code** with the PlatformIO IDE extension installed.
2. Edit configuration parameters in `include/config.h` (Wi-Fi SSID, passwords, Blynk Auth token, sensor calibration ranges).
3. Connect the ESP32 DevKit to your machine via Micro-USB.
4. Build and upload using PlatformIO's project tasks:
   - **Build**: `pio run`
   - **Upload**: `pio run --target upload`
   - **Monitor**: `pio run --target monitor`

---

## 🖥️ Web Control Room

The dashboard is built entirely with vanilla CSS and JavaScript, meaning it requires no bundlers or complex build setups.

* Simply open `index.html` in any modern desktop, tablet, or mobile web browser.
* It will auto-discover the local ESP32 IP address or MDNS host `flura.local`.
* You can toggle manual/automatic pump states, set moisture thresholds, and view high-fidelity real-time telemetry.

---

## 📄 Documentation Index
* [instruction.md](file:///F:/Work%20for%20Ankita/Aqua_nexus_Pro_V2.4.3/instruction.md) — Comprehensive build instructions.
* [component_roles.md](file:///F:/Work%20for%20Ankita/Aqua_nexus_Pro_V2.4.3/component_roles.md) — Detailed breakdown of every component's function.
* [pin_connection_details.md](file:///F:/Work%20for%20Ankita/Aqua_nexus_Pro_V2.4.3/pin_connection_details.md) — Wire-level routing tables.
* [schematic_diagram.md](file:///F:/Work%20for%20Ankita/Aqua_nexus_Pro_V2.4.3/schematic_diagram.md) — Structural diagrams and circuit details.
* [blynk_credentials.md](file:///F:/Work%20for%20Ankita/Aqua_nexus_Pro_V2.4.3/blynk_credentials.md) — Setup guide for Blynk virtual pins.

---

## 👥 Project Team & Contributors
* **Ankita Das** — Project Lead
* **Ananya Chakraborty** — Hardware Systems
* **Dipankar Saha** — Systems Engineering
* **Debraj Ghosh** — QA & Assembly
* **Asif Sarwar** — Firmware Design
* **MX SOURAV** — Dashboard Designer & Developer
