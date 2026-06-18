# AquaNexus Pro - Smart Irrigation System

AquaNexus Pro is an automated smart planter and irrigation control system built around the 30-pin ESP32 microcontroller. The system features a local web control panel communicating via WebSockets and integrates with the Blynk IoT Cloud for remote access.

It monitors soil moisture levels and water tank depth, managing a DC water pump automatically based on configured thresholds.

---

## Hardware Specifications

### Power System (Dual Isolated Source, Shared Ground)
- **ESP32 Power Source (Battery System)**:
  - Cells: Two 3.7V Lithium-Ion battery cells (18650 nominal, connected in parallel to provide 3.7V-4.2V).
  - Charger: TP4056 Lithium Battery Charger Module (charges battery cells).
  - Boost Converter: MT3608 DC-DC Boost Converter Module (steps up 3V-4.2V battery voltage to 5V to power the ESP32 VIN).
- **Water Pump Power Source (Adapter System)**:
  - Adapter: 12V DC Power Adapter (supplies independent power to the pump).
  - Water Pump: DC Water Pump Motor.
- **Shared Ground**: Both the battery system (boost converter output GND) and the adapter system (12V adapter negative) are tied together to a single common GND bus.

### Controller & Logic
- MCU: ESP32 DevKit V1 (30-pin version)
- MOSFET: IRLZ44N N-channel Logic-Level MOSFET (controls the pump)
- Diode: 1N4007 Rectifier Diode (Flyback protection across the pump terminals)
- Resistors:
  - 100k Ohm (MOSFET Gate pull-down resistor to GND)
  - 1k Ohm & 2k Ohm (Voltage divider on the HC-SR04 Echo pin to drop 5V to 3.3V)
- Capacitors:
  - 100uF (Electrolytic power filter capacitor on the ESP32 5V input)
  - 100nF / 104 (Ceramic decoupling capacitor on the ESP32 5V input)
- Display: 1.8" SPI TFT ST7735
- Sensors:
  - HC-SR04 Ultrasonic Sensor (water level)
  - Capacitive Soil Moisture Sensor v1.2
- Alarm: 5V Active Buzzer
- Input: Tactile Push Button

---

## Detailed Pin Connection Diagram

All components must share a common ground connection.

| Module / Component | Component Pin | Connection Destination | Hardware Component / Notes |
| --- | --- | --- | --- |
| **Lithium Battery Pack** | Joined Positive (+) | TP4056 B+ | Two lithium cells in parallel |
| | Joined Negative (-) | TP4056 B- | Ground path |
| **TP4056 Charger** | B+ / B- | Battery Positive / Negative | Charging connections |
| | OUT+ | MT3608 VIN+ | Output to boost converter input |
| | OUT- | MT3608 VIN- / GND | Output ground |
| **MT3608 Boost Module** | VIN+ / VIN- | TP4056 OUT+ / OUT- | Input from charger board |
| | VOUT+ (5V) | ESP32 VIN | Steps up battery voltage to 5V |
| | VOUT- (GND) | GND Bus | Connects to common ground |
| **12V Power Adapter** | Positive (+) | Pump Positive (+) | Directly powers the water pump |
| | Negative (-) | GND Bus | Tied to common ground |
| **ESP32 (30-pin)** | VIN | MT3608 VOUT+ (5V) | Main 5V power input |
| | GND | GND Bus | Common Ground |
| | 3.3V | VCC Bus (3.3V) | Powers Moisture Sensor and TFT VCC |
| **DC Water Pump** | Positive (+) | 12V Bus (Adapter +) | Powered directly by the 12V supply |
| | Negative (-) | MOSFET Drain | Controlled switching path |
| **1N4007 Diode** | Cathode (Stripe) | 12V Bus (Adapter +) | Connected across pump positive terminal |
| | Anode | MOSFET Drain | Connected across pump negative terminal |
| **IRLZ44N MOSFET** | Gate (G1) | ESP32 GPIO 25 | PWM pump speed control |
| | Gate (G2) | 100k Ohm Resistor -> GND | Gate pull-down resistor |
| | Drain (D) | Pump Negative (-) | Switched path |
| | Source (S) | GND Bus | Connected to common ground |
| **HC-SR04 Ultrasonic** | VCC | MT3608 VOUT+ (5V) | Powered by 5V |
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
| | 104 Ceramic | ESP32 VIN & GND | Parallel high-frequency decoupling |

---

## Power Distribution Layout

1. **ESP32 5V Power Path**: Power is supplied by two parallel lithium cells charging via a TP4056 module. The battery voltage is stepped up to 5V by the MT3608 boost converter and sent to the ESP32 VIN pin.
2. **Pump 12V Power Path**: The DC water pump is powered independently by a 12V power adapter.
3. **Common Ground Plane**: The negative output of the MT3608 boost converter, the negative terminal of the 12V adapter, the ESP32 GND, and the MOSFET source pin are all tied to a single common GND. This common reference is required for the ESP32 to switch the MOSFET.
4. **Inductive Flyback Protection**: The 1N4007 diode is connected across the water pump terminals (cathode to 12V positive, anode to MOSFET drain) to suppress inductive voltage spikes generated when the pump turns off.
5. **Gate Control**: The 100k Ohm gate pull-down resistor holds the MOSFET gate at GND to prevent the pump from turning on during ESP32 bootup or when the GPIO pin is in a floating state.
6. **ECHO Voltage Divider**: The HC-SR04 sensor outputs a 5V echo signal. To prevent damage to the 3.3V logic of the ESP32, a voltage divider (1k Ohm series resistor and 2k Ohm pull-down resistor) steps the echo signal down to 3.3V before it enters GPIO 35.

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
