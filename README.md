
# Toki Gachi

> A compact, multifunctional ESP32-based handheld device combining retro gaming, wireless utilities, Bluetooth HID, infrared control, and SD card storage.

![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B-orange)
![Display](https://img.shields.io/badge/Display-SH1106%20OLED-green)
![Status](https://img.shields.io/badge/Status-Development-yellow)

---

## Overview

**Toki Gachi** is an embedded electronics project built around an ESP32 microcontroller and a 128×64 SH1106 OLED display.

The project integrates multiple functions into a single handheld interface, controlled through five physical buttons. It combines entertainment, wireless experimentation, infrared remote control, and storage management.

The device is designed as a practical exploration of embedded systems, human-machine interaction, wireless technologies, and hardware-software integration.

---

## Features

### Music Player
- RTTTL-based melody playback.
- Multiple built-in songs.
- Animated music visualization.
- Buzzer-based audio output.
- Song selection through the OLED interface.

### Snake Game
- Classic Snake gameplay.
- Directional button controls.
- Food spawning and score tracking.
- High-score tracking.
- Increasing difficulty as the score rises.
- Game-over and pause states.

### Wi-Fi Scanner
- Scans for nearby Wi-Fi networks.
- Displays detected SSIDs.
- Shows RSSI signal strength.
- Indicates whether a network is open or secured.
- Asynchronous scanning with timeout handling.
- OLED-based network browsing.

### Bluetooth HID Mouse
- Wireless mouse control through Bluetooth HID.
- Cursor movement using physical buttons.
- Left click, right click, and double click.
- Drag functionality through a long press.
- Scrolling mode.
- Connection status displayed on the OLED.

### IR Remote
- Infrared signal learning.
- Support for decoded IR protocols.
- Raw IR signal capture.
- Storage of learned IR codes.
- Infrared transmission.
- Multiple configurable IR slots.

### SD Card Tools
- SD card initialization.
- File listing.
- File deletion.
- SD card formatting with confirmation.
- Saving and loading IR codes.
- SPI-based SD card communication.

### Wi-Fi Security Research
- Wi-Fi deauthentication detection.
- Wireless monitoring and detection statistics.
- Channel selection and attacker tracking.
- Attack logging and detection alerts.

> **Responsible Use:** Wireless security functionality should only be used on networks and devices you own or have explicit permission to test. Follow applicable laws and avoid disrupting other people's communications.

---

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 development board | Main microcontroller |
| SH1106 128×64 OLED | User interface |
| 5 Push buttons | Device navigation and controls |
| Buzzer | Music and notification sounds |
| IR receiver | Infrared signal capture |
| IR LED/transmitter | Infrared signal transmission |
| MicroSD card module | Data storage |
| MicroSD card | File and IR code storage |

> **Hardware Note:** Verify the target ESP32 board and its GPIO mapping before assembly. The provided source uses GPIO assignments that may not be available on every ESP32 variant.

---

## Software Libraries

The project uses the following libraries:

- [Arduino Core for ESP32](https://github.com/espressif/arduino-esp32)
- [U8g2](https://github.com/olikraus/u8g2) — OLED graphics
- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) — Infrared communication
- HijelHID_BLEMouse — Bluetooth HID mouse functionality
- ESP32 Wi-Fi libraries
- SD and SPI libraries

## Controls

### Main Menu

| Button | Function |
|---|---|
| UP / DOWN | Navigate options |
| LEFT / RIGHT | Change selection |
| SELECT | Open selected feature |

### Music Player

| Button | Function |
|---|---|
| UP / DOWN | Scroll songs |
| SELECT | Play / stop |
| LEFT | Return |

### Snake

| Button | Function |
|---|---|
| Direction buttons | Move snake |
| SELECT | Pause / resume / restart |
| LEFT | Return to menu |

### Bluetooth Mouse

| Control | Function |
|---|---|
| UP / DOWN / LEFT / RIGHT | Move cursor |
| SELECT tap | Left click |
| SELECT double tap | Right click |
| SELECT triple tap | Double click |
| SELECT hold | Drag |
| SELECT + UP hold | Enter scroll mode |
| Long LEFT | Return to menu |

> Controls may vary with the implementation and firmware version.

---

## Future Improvements

Potential improvements for future versions:

- [ ] Improve menu animations.
- [ ] Add customizable music.
- [ ] Expand IR code management.
- [ ] Add battery monitoring.
- [ ] Improve power management.
- [ ] Add more mini-games.
- [ ] Improve modularity of the firmware.
- [ ] Add a dedicated hardware enclosure.
- [ ] Improve wireless security detection reliability.
- [ ] Add a more comprehensive settings menu.

---

## Disclaimer

This project is intended for educational purposes, embedded systems experimentation, and authorized wireless security research.

The developer is not responsible for misuse of the hardware or software. Always follow applicable laws and obtain permission before interacting with networks, devices, or signals that you do not own.

---

## Author

**Ahnaf Hossain**

Electronics and Embedded Systems Enthusiast.

**Ahsanullah University of Science and Technology**
