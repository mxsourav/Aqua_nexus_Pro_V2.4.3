# AquaNexus Pro - Smart Irrigation System

AquaNexus Pro is an automated smart planter and irrigation control system built around the 30-pin ESP32 microcontroller. The system features a local web control panel communicating via WebSockets and integrates with the Blynk IoT Cloud for remote access.

It monitors soil moisture levels and water tank depth, managing a 12V DC water pump automatically based on configured thresholds.

---

## Hardware Specifications

- MCU: ESP32 DevKit V1 (30-pin version)
- Power Supply: 12V 1A DC Power Adapter
- Water Pump: 12V DC Motor
- MOSFET: IRLZ44N N-channel Logic-Level MOSFET (Pump Driver)
- Diode: 1N4007 Rectifier Diode (Flyback protection across pump)
- Resistors:
  - 100k Ohm (MOSFET Gate pull-down)
  - 1k Ohm & 2k Ohm (Ultrasonic Echo voltage divider)
- Capacitors:
  - 100uF (Electrolytic power filter capacitor, 25V or higher)
  - 100nF / 104 (Ceramic decoupling capacitor)
- Display: 1.8" SPI TFT ST7735
- Water Level Sensor: HC-SR04 Ultrasonic Sensor
- Moisture Sensor: Capacitive Soil Moisture Sensor v1.2
- Alarm: 5V Active Buzzer
- Input: Tactile Push Button

---

## Detailed Pin Connection Diagram

All components must share a common ground connection.

| Module / Component | Component Pin | Connection Destination | Hardware Component / Notes |
| --- | --- | --- | --- |
| **12V 1A Adapter** | Positive (+) | 12V Bus | Connects to 12V pump positive and Buck Converter VIN+ |
| | Negative (-) | GND Bus | Common Ground return path |
| **5V Step-Down (Buck / 7805)** | VIN+ | 12V Bus | Input from 12V power adapter |
| | VIN- / GND | GND Bus | Input ground |
| | VOUT+ (5V) | ESP32 VIN | Steps down 12V to 5V to power the ESP32 |
| | VOUT- (GND) | GND Bus | Steps down ground |
| **ESP32 (30-pin)** | VIN | 5V Bus (Buck VOUT+) | Main 5V power input |
| | GND | GND Bus | Common Ground |
| | 3.3V | VCC Bus (3.3V) | Powers Moisture Sensor and TFT Display VCC |
| **12V DC Pump (Motor)** | Positive (+) | 12V Bus | Connected to 12V positive supply |
| | Negative (-) | MOSFET Drain | Controlled switching path |
| **1N4007 Diode** | Cathode (Stripe) | 12V Bus | Connected across pump positive terminal |
| | Anode | MOSFET Drain | Connected across pump negative terminal |
| **IRLZ44N MOSFET** | Gate (G1) | ESP32 GPIO 25 | PWM pump speed control |
| | Gate (G2) | 100k Ohm Resistor -> GND | Gate pull-down resistor to prevent pump float |
| | Drain (D) | Pump Negative (-) | Switched path |
| | Source (S) | GND Bus | Connected to common ground |
| **HC-SR04 Ultrasonic** | VCC | 5V Bus | Powered by 5V |
| | GND | GND Bus | Common Ground |
| | TRIG | ESP32 GPIO 33 | Trigger signal |
| | ECHO | 1k Ohm Resistor | Connected to 1k Ohm resistor of the voltage divider |
| **ECHO Voltage Divider** | Resistor 1 (1k) | HC-SR04 ECHO | Series resistor |
| | Junction | ESP32 GPIO 35 | Junction between 1k and 2k resistors |
| | Resistor 2 (2k) | GND Bus | Pull-down to GND (reduces 5V echo to 3.3V) |
| **Moisture Sensor v1.2** | VCC | ESP32 3.3V | Powered by 3.3V |
| | GND | GND Bus | Common Ground |
| | AOUT | ESP32 GPIO 34 | Analog moisture reading (ADC1 CH6) |
| **ST7735 TFT Display** | VCC | ESP32 3.3V | Power input |
| | GND | GND Bus | Common Ground |
| | CS | ESP32 GPIO 5 | SPI Chip Select |
| | RST | ESP32 GPIO 4 | Display Reset |
| | A0 / DC | ESP32 GPIO 22 | Data / Command Selection |
| | SDA / MOSI | ESP32 GPIO 23 | SPI Master Out Slave In |
| | SCK / CLK | ESP32 GPIO 18 | SPI Clock |
| | LED / BL | ESP32 GPIO 21 | Backlight control |
| **Active Buzzer (5V)** | Positive (+) | ESP32 GPIO 26 | Alarm output |
| | Negative (-) | GND Bus | Common Ground |
| **Tactile Push Button** | Pin 1 | ESP32 GPIO 27 | Menu toggle input (uses internal pull-up) |
| | Pin 2 | GND Bus | Ground connection |
| **Power Capacitors** | 100uF (+) | ESP32 VIN (5V) | Filter capacitor (observe polarity) |
| | 100uF (-) | GND Bus | Filter capacitor ground |
| | 104 Ceramic | ESP32 VIN & GND | Parallel high-frequency decoupling (no polarity) |

---

## Power Distribution Layout

1. **12V Rail**: The 12V 1A adapter supplies power to the 12V DC water pump and a 5V buck converter (or LM7805 regulator).
2. **5V Rail**: The 5V output from the buck converter connects directly to the ESP32 VIN pin, the HC-SR04 VCC, and the Active Buzzer positive terminal.
3. **3.3V Rail**: The ESP32 onboard regulator provides 3.3V output to power the ST7735 TFT display and the Capacitive Soil Moisture Sensor.
4. **Inductive Load Protection**: The 1N4007 diode is wired in parallel with the 12V pump (cathode to positive supply, anode to MOSFET drain). This protects the MOSFET from inductive voltage spikes when the pump turns off.
5. **Decoupling and Noise Filtering**: The 100uF electrolytic capacitor and the 100nF ceramic capacitor are connected in parallel across the ESP32's VIN and GND pins to filter out electrical noise caused by the pump startup.

---

## Setup and Installation

### 1. Firmware Configuration
1. Open this project directory in VS Code with the PlatformIO extension installed.
2. Rename `include/secrets.h.example` to `include/secrets.h`.
3. Add your Wi-Fi SSID, password, and Blynk Auth token to `include/secrets.h`.
4. Connect the 30-pin ESP32 to your PC using a micro-USB cable.
5. Compile and upload the firmware using the PlatformIO interface.

### 2. Web Interface Deployment
1. Open `index.html` in any web browser.
2. The page connects to the ESP32 WebSocket server using the default local mDNS address (`flura.local`) or the local IP address of your ESP32.

---

## Developer

Developed by MX SOURAV (https://github.com/mxsourav)
