# Project Flour Junior Handoff

This zip has only the files needed to understand the project, open the website, and rebuild/upload the firmware from PlatformIO.

## What is inside this zip

- `platformio.ini`
- `include/`
- `src/`
- `index.html`
- `instruction.md`
- `pin_connection_details.md`
- `component_roles.md`
- `blynk_credentials.md`
- `schematic_diagram.md`
- `Aqua_Nexus_Pro_V2.4.3_Schematic.pdf`

## What is not inside this zip

- No `.bin` file
- No `.pio` build folder
- No upload logs
- No APK
- No extra screenshots or unused files

## Before starting

1. Install VS Code.
2. Install the PlatformIO extension in VS Code.
3. Keep the ESP32 USB driver ready if the laptop does not detect the board.
4. Open this project folder in VS Code.

## Folder use

- `src/` has the main firmware code.
- `include/` has pin mapping, Wi-Fi details, thresholds, and helper declarations.
- `index.html` is the working browser dashboard file for ESP32 WebSocket control.

## Basic build and upload steps

1. Open the project folder in VS Code.
2. Wait for PlatformIO to load the project.
3. Connect the ESP32 board with USB.
4. Check the COM port in Device Manager.
5. Open `platformio.ini`.
6. Update `upload_port` and `monitor_port` if your COM port is different.
7. Build the project.
8. Upload the project.
9. Open serial monitor at `115200`.

## Wi-Fi and app setup

1. Open `include/config.h`.
2. Change `WIFI_SSID` and `WIFI_PASSWORD` if needed.
3. Check `DEVICE_HOSTNAME`. Right now it is set to `flura`.
4. Blynk credentials and pin mapping are also written in `blynk_credentials.md`.
5. If Blynk is being used on another network or account, update the Blynk template and token values.

## Website use

1. Power the ESP32 and let it connect to Wi-Fi.
2. Open `index.html` in a browser on the same network.
3. The page tries auto-connect using the saved IP and `flura.local`.
4. If auto-connect does not work, type the ESP32 IP manually and press connect.

## Assembly order I would suggest

1. First make the power section.
2. Then connect ESP32 power and common ground.
3. Then connect the pump driver section with MOSFET and flyback diode.
4. Then connect the moisture sensor.
5. Then connect the ultrasonic sensor with the voltage divider on echo.
6. Then connect the TFT display.
7. Then connect the buzzer and button.
8. Only after wiring check, power it on.
9. Upload firmware after checking wiring once again.

## Testing order

1. Test ESP32 power only.
2. Test TFT power and display startup.
3. Test moisture readings in serial monitor.
4. Test website connection.
5. Test pump with a short burst.
6. Test buzzer.
7. Test tank-empty safety only after the full wiring is checked.

## Important notes

- The current firmware package is the working one that was used last.
- The website file to use is the root `index.html` in this zip, not the other dashboard HTML from the old folder.
- Keep all grounds common. ESP32 ground, sensor ground, pump ground, TP4056 ground, and booster ground must be connected together.
- The HC-SR04 echo line must not go directly to ESP32 if it is giving 5V logic. Use the divider mentioned in the pin connection file.

## Precautions

- Wrong battery can fry the ESP32 permanently. Choose the right battery carefully.
- If you change battery type, pack voltage, or power path, check the full power section again and modify the firmware/settings only after verifying the new supply is safe for the board and modules.
- Do not give raw battery voltage directly to ESP32 `3.3V` pin.
- Do not connect the pump directly to an ESP32 GPIO pin.
- Do not run the pump without flyback diode.
- Do not connect the soil moisture sensor `VCC` to the wrong supply by guesswork. Check first.
- Do not skip the ground connection.
- Do not reverse diode polarity.
- Do not connect the ultrasonic echo directly if the module is outputting 5V.
- Disconnect power before changing wiring.

## One real firmware note

- In `src/main.cpp`, the `readUltrasonicSensor()` section is currently set in a simplified testing state where the tank is forced to look full. If someone wants real live tank-distance logic again, that function is the place to restore first.

## Final check before demo

1. ESP32 boots properly.
2. TFT turns on.
3. Website connects.
4. Moisture value updates.
5. Pump responds in manual mode.
6. Auto mode threshold works.
7. Buzzer works.
8. No loose power wires.
