# Project Flour Schematic Diagram

This is a text schematic for the actual project version in this zip. It is not copied from the sample project. It is based on the current firmware pin map and the final wiring used here.

## 1. System block view

```text
2x 18650 Parallel Pack
        |
        v
     TP4056 Charger
        |
        v
   MT3608 Boost Module
        |
        +---------------------------> ESP32 VIN / 5V
        |                                   |
        |                                   +--> TFT Display
        |                                   +--> Buzzer
        |                                   +--> Logic and Wi-Fi
        |
        +--> Pump + ------------------------------+
                                                  |
                                             Water Pump
                                                  |
                                                  +--> MOSFET Drain

ESP32 GPIO25 ----> MOSFET Gate
MOSFET Source ---> GND
Pump flyback diode across pump terminals

ESP32 GPIO34 <---- Soil Moisture Sensor AOUT
ESP32 GPIO33 ----> HC-SR04 TRIG
ESP32 GPIO35 <---- HC-SR04 ECHO through 1k/2k divider
ESP32 GPIO26 ----> Active Buzzer
ESP32 GPIO27 <---- Push Button to GND
ESP32 GPIO13 <---- Optional Float Switch

ESP32 SPI Lines ----> ST7735 TFT
GPIO23 MOSI
GPIO18 SCK
GPIO5  CS
GPIO22 DC
GPIO4  RST
GPIO21 BL
```

## 2. Detailed connection-style schematic

```text
                                 +----------------------+
                                 |    2 x 18650 Cells   |
                                 |      in parallel     |
                                 +----------+-----------+
                                            |
                                   B+ ------+------ B-
                                            |
                                     +------+------+
                                     |    TP4056   |
                                     | Charger     |
                                     +------+------+
                                            |
                              OUT+ ---------+--------- OUT-
                                            |
                                     +------+------+
                                     |    MT3608   |
                                     | Boost Conv. |
                                     +------+------+
                                            |
                 +--------------------------+--------------------------+
                 |                                                     |
              VOUT+                                                  VOUT-
                 |                                                     |
                 |                                                     +------------------ GND BUS
                 |
         +-------+------------------------------+
         |                                      |
         |                                      |
     ESP32 VIN / 5V                        Pump Positive
         |
   +-----+--------------------------------------------------------------+
   |                      ESP32 DEVKIT                                   |
   |                                                                     |
   | GPIO34 <---------------- Soil Moisture Sensor AOUT                  |
   | GPIO33 ----------------> HC-SR04 TRIG                               |
   | GPIO35 <---------------- HC-SR04 ECHO via divider                   |
   | GPIO25 ----------------> IRLZ44N Gate                               |
   | GPIO26 ----------------> Active Buzzer +                            |
   | GPIO27 <---------------- Push Button                                |
   | GPIO13 <---------------- Optional Float Switch                      |
   |                                                                     |
   | GPIO23 ----------------> TFT MOSI / SDA                             |
   | GPIO18 ----------------> TFT SCK / CLK                              |
   | GPIO5  ----------------> TFT CS                                     |
   | GPIO22 ----------------> TFT DC                                     |
   | GPIO4  ----------------> TFT RST                                    |
   | GPIO21 ----------------> TFT BL / LED                               |
   |                                                                     |
   | 3.3V ------------------> Soil Moisture Sensor VCC                   |
   | 3.3V or module VCC -----> TFT VCC                                   |
   | GND --------------------> Common GND                                |
   +---------------------------------------------------------------------+


Pump switching stage:

ESP32 GPIO25 ----[Gate] IRLZ44N [Source]---- GND
                     |
                  [100k]
                     |
                    GND

Pump negative ----------------------[Drain] IRLZ44N
Pump positive ----------------------------------- MT3608 VOUT+

Flyback diode across pump:
Anode  -> Pump negative side
Cathode -> Pump positive side


HC-SR04 echo divider:

HC-SR04 ECHO ----[1k]----+----> ESP32 GPIO35
                         |
                        [2k]
                         |
                        GND


Power filtering:

ESP32 VIN/5V ----+----[100uF electrolytic]---- GND
                 |
                 +----[104 ceramic capacitor]- GND
```

## 3. Notes
- The firmware hostname is `flura`, so browser dashboard access is expected through `flura.local`.
- The website WebSocket endpoint is `ws://flura.local:81/ws`.
- The ultrasonic section in current firmware is temporarily fixed to full-tank behavior for testing, even though the wiring is still documented correctly here.
