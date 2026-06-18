# Project Flour Pin Connection Details

This is the full wiring reference for the final build.

## 1. Power section

### Battery pack

- Use `2 x 18650` cells in parallel.
- Battery 1 positive goes to Battery 2 positive.
- Battery 1 negative goes to Battery 2 negative.
- The joined positive goes to `TP4056 B+`.
- The joined negative goes to `TP4056 B-`.

### TP4056 charging board

- `B+` to battery positive.
- `B-` to battery negative.
- `OUT+` to `MT3608 VIN+`.
- `OUT-` to `MT3608 VIN-`.

### MT3608 boost converter

- `VIN+` from `TP4056 OUT+`.
- `VIN-` from `TP4056 OUT-`.
- Adjust output before connecting ESP32.
- `VOUT+` goes to ESP32 `5V` or `VIN`.
- `VOUT-` goes to ESP32 `GND`.

### Power capacitors

- `100uF electrolytic capacitor` across ESP32 power input and GND.
- Positive leg of capacitor to `5V/VIN side`.
- Negative leg of capacitor to `GND`.
- `104 ceramic capacitor` in parallel across ESP32 power input and GND.
- Ceramic capacitor has no polarity.

## 2. ESP32 main power

- ESP32 `5V/VIN` gets boosted supply from MT3608 `VOUT+`.
- ESP32 `GND` goes to MT3608 `VOUT-`.

## 3. Soil moisture sensor

- Sensor `VCC` to ESP32 `3.3V`.
- Sensor `GND` to common `GND`.
- Sensor `AOUT` to ESP32 `GPIO 34`.

## 4. Ultrasonic sensor HC-SR04

- `VCC` to `5V`.
- `GND` to common `GND`.
- `TRIG` to ESP32 `GPIO 33`.
- `ECHO` goes to ESP32 `GPIO 35` through voltage divider.

### Echo voltage divider

- Put `1k ohm` resistor between `HC-SR04 ECHO` and ESP32 `GPIO 35`.
- Put `2k ohm` resistor from ESP32 `GPIO 35` to `GND`.
- If `2k ohm` is not available, use two `1k ohm` resistors in series to make `2k ohm`.
- This divider is needed so ESP32 does not get hit with full `5V` on echo.

## 5. Pump driver section

### IRLZ44N MOSFET

- Gate to ESP32 `GPIO 25`.
- Gate to `GND` also through a `100k ohm` pull-down resistor.
- Drain to pump negative wire.
- Source to common `GND`.

### Pump supply

- Pump positive wire goes to `MT3608 VOUT+`.
- Pump negative wire goes to MOSFET drain.

### Flyback diode

- Use `1N4007`.
- Connect across pump terminals.
- Diode stripe side goes to pump positive side.
- Other diode side goes to pump negative side.

## 6. TFT display ST7735

- `VCC` to `3.3V` or `5V` only if the display module has onboard regulator support.
- `GND` to common `GND`.
- `CS` to ESP32 `GPIO 5`.
- `RST` to ESP32 `GPIO 4`.
- `DC` to ESP32 `GPIO 22`.
- `MOSI` or `SDA` to ESP32 `GPIO 23`.
- `SCK` or `CLK` to ESP32 `GPIO 18`.
- `BL` or `LED` to ESP32 `GPIO 21`.

## 7. Push button

- One side of button to ESP32 `GPIO 27`.
- Other side of button to `GND`.
- Internal pull-up is already used in firmware.

## 8. Buzzer

- Buzzer positive to ESP32 `GPIO 26`.
- Buzzer negative to `GND`.

## 9. Float switch

- Float switch line to ESP32 `GPIO 13`.
- Other side to `GND` depending on module style.
- Current firmware has float switch support disabled, but the pin is still reserved in config.

## 10. Common ground rule

- TP4056 `OUT-`
- MT3608 `VIN-`
- MT3608 `VOUT-`
- ESP32 `GND`
- Pump negative return path
- Moisture sensor `GND`
- HC-SR04 `GND`
- TFT `GND`
- Buzzer `GND`

All of these must be connected to the same common ground.

## 11. Quick pin table

| Part | Signal | ESP32 Pin |
| --- | --- | --- |
| Moisture Sensor | AOUT | GPIO 34 |
| HC-SR04 | TRIG | GPIO 33 |
| HC-SR04 | ECHO | GPIO 35 |
| Pump MOSFET | GATE | GPIO 25 |
| Buzzer | + | GPIO 26 |
| Button | Signal | GPIO 27 |
| Float Switch | Signal | GPIO 13 |
| TFT | CS | GPIO 5 |
| TFT | RST | GPIO 4 |
| TFT | DC | GPIO 22 |
| TFT | MOSI | GPIO 23 |
| TFT | SCK | GPIO 18 |
| TFT | BL | GPIO 21 |

## 12. Wiring checks before power on

1. Check polarity of battery.
2. Check polarity of electrolytic capacitor.
3. Check polarity of flyback diode.
4. Check ESP32 is not getting raw battery directly on wrong pin.
5. Check echo divider is present.
6. Check pump is not connected directly to GPIO.
7. Check all grounds are common.
