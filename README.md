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

## Pin Connection Tables by Module

All components must share a common ground reference.

### 1. Power Supply, Charger & Boost Converter

| Component | Pin | Connected To | Notes |
| --- | --- | --- | --- |
| **Lithium Battery Pack** | Positive (+) | TP4056 B+ | Two lithium cells in parallel |
| | Negative (-) | TP4056 B- | Battery ground path |
| **TP4056 Charger** | B+ / B- | Battery (+) / (-) | Input charging connection |
| | OUT+ | MT3608 VIN+ | Positive output path |
| | OUT- | MT3608 VIN- | Return output ground path |
| **MT3608 Boost Module** | VIN+ / VIN- | TP4056 OUT+ / OUT- | input from charger |
| | VOUT+ | ESP32 VIN (5V) | Regulated 5V output to ESP32 |
| | VOUT- | GND Bus | Connects to common ground |
| **12V Power Adapter** | Positive (+) | Pump Positive (+) | Switched 12V bus for water pump |
| | Negative (-) | GND Bus | Tied to common ground |

### 2. IRLZ44N MOSFET & 12V DC Pump Switching

| Component / Node | Pin / Terminal | Connected To | Notes |
| --- | --- | --- | --- |
| **ESP32** | GPIO 25 | MOSFET Gate | PWM signal controlling pump speed |
| **IRLZ44N MOSFET** | Gate | ESP32 GPIO 25 | Gate terminal |
| | Gate | 100k Ohm Resistor -> GND | Pulled down to GND to prevent boot float |
| | Drain | Pump Negative (-) | Switched ground return path |
| | Source | GND Bus | Connected directly to common ground |
| **DC Water Pump** | Positive (+) | 12V Bus (Adapter +) | Directly powered by 12V supply |
| | Negative (-) | MOSFET Drain | Switching return path |
| **1N4007 Diode** | Cathode (Stripe) | 12V Bus (Adapter +) | Clamps voltage spikes |
| | Anode | MOSFET Drain | Clamps voltage spikes |

### 3. HC-SR04 Ultrasonic Sensor & Echo Divider

| Component | Pin / Resistor | Connected To | Notes |
| --- | --- | --- | --- |
| **HC-SR04 Sensor** | VCC | MT3608 VOUT+ (5V) | Main 5V power input |
| | GND | GND Bus | Common Ground |
| | TRIG | ESP32 GPIO 33 | Trigger signal from ESP32 |
| | ECHO | 1k Ohm Resistor | Output signal |
| **Echo Divider** | 1k Ohm Resistor | HC-SR04 ECHO pin | In series with ECHO pin |
| | 2k Ohm Resistor | GND Bus | Pull-down resistor |
| | Junction | ESP32 GPIO 35 | Drops 5V echo output to safe 3.3V |

### 4. Capacitive Soil Moisture Sensor v1.2

| Component | Pin | Connected To | Notes |
| --- | --- | --- | --- |
| **Moisture Sensor** | VCC | ESP32 3.3V | Powered by 3.3V to minimize sensor corrosion |
| | GND | GND Bus | Common Ground |
| | AOUT | ESP32 GPIO 34 | Analog soil moisture signal (ADC1 CH6) |

### 5. ST7735 1.8" TFT Display (SPI)

| Component | Pin | Connected To | Notes |
| --- | --- | --- | --- |
| **TFT Display** | VCC | ESP32 3.3V | Power input |
| | GND | GND Bus | Common Ground |
| | CS | ESP32 GPIO 5 | SPI Chip Select |
| | RST | ESP32 GPIO 4 | Display hardware reset |
| | A0 / DC | ESP32 GPIO 22 | Data / Command select line |
| | SDA / MOSI | ESP32 GPIO 23 | SPI Master Out Slave In |
| | SCK / CLK | ESP32 GPIO 18 | SPI Clock |
| | LED / BL | ESP32 GPIO 21 | Backlight control pin |

### 6. Buzzer, Button & Power Filtering Capacitors

| Component | Pin | Connected To | Notes |
| --- | --- | --- | --- |
| **5V Active Buzzer** | Positive (+) | ESP32 GPIO 26 | Alarm control signal |
| | Negative (-) | GND Bus | Ground return |
| **Tactile Push Button** | Pin 1 | ESP32 GPIO 27 | Mode toggle (configured with internal pull-up) |
| | Pin 2 | GND Bus | Ground connection |
| **Electrolytic Cap** | Positive (+) | ESP32 VIN (5V) | 100uF smoothing capacitor (observe polarity) |
| | Negative (-) | GND Bus | Ground path |
| **Ceramic Cap** | Non-polar (104) | ESP32 VIN & GND | 100nF high-frequency decoupling capacitor |

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
