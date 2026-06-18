# Project Flour Component Roles

This file explains what each component does in the project.

## ESP32 DevKit

This is the main controller. It reads sensors, decides when to run the pump, drives the display, runs Wi-Fi, talks to Blynk, and serves the WebSocket connection used by the website.

## Capacitive soil moisture sensor

This sensor gives the soil moisture level as an analog voltage. The ESP32 reads that value on `GPIO 34` and converts it into a percentage.

## HC-SR04 ultrasonic sensor

This checks the water level in the tank by measuring distance from the sensor to the water surface. Bigger distance means less water in the tank.

## IRLZ44N MOSFET

This is the electronic switch for the pump. The ESP32 GPIO cannot drive the pump directly, so the MOSFET does the actual high-current switching.

## Water pump

This moves water from the tank to the plant. It is the load being controlled by the MOSFET.

## 1N4007 diode

This is the flyback protection diode across the pump. When the pump turns off, the motor can create reverse voltage spikes. This diode absorbs that effect and protects the MOSFET and ESP32 side.

## 100k ohm gate pull-down resistor

This resistor keeps the MOSFET gate low when the ESP32 pin is not actively driving it. Without this, the gate can float and the pump may turn on randomly.

## 1k ohm and 2k ohm resistors on ultrasonic echo

These resistors make the voltage divider for the HC-SR04 echo pin. Their job is to step the echo signal down before it reaches the ESP32.

## TP4056 module

This is the lithium battery charging board. It charges the 18650 cells safely and also provides battery output connections.

## MT3608 boost converter

This board boosts the battery voltage up to the level required by the ESP32 input and pump power line.

## 18650 batteries

These are the main power source. In this project they are used in parallel to improve backup time while keeping voltage in the same range.

## 100uF electrolytic capacitor

This capacitor helps smooth sudden voltage dips and power noise, especially when the pump starts or when the board current changes quickly.

## 104 ceramic capacitor

This small capacitor helps filter higher-frequency noise on the supply line. It works together with the bigger electrolytic capacitor.

## ST7735 TFT display

This shows live project status like moisture, water level, Wi-Fi details, and system state directly on the hardware.

## Push button

This button changes the display view on the TFT.

## Active buzzer

This gives sound alerts and UI beeps for actions like alarm or feedback.

## Float switch

This is an optional extra water-level safety input. In the current code it is reserved, but not actively enabled by default.

## Website `index.html`

This is the browser dashboard. It connects to the ESP32 through WebSocket on port `81` and gives manual control plus live status view.

## Blynk

This is the cloud/mobile control side. It is used for remote status, events, threshold control, and manual pump actions.
