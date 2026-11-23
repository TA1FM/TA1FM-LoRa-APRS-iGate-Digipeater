# TA1FM LoRa APRS iGate & Digi v2.0.0-beta.1

**BETA / field testing — not a stable release.** Tested on Heltec Wireless Stick Lite V3 (ESP32-S3, SX1262). This beta source is based on the locally tested DEV7.2 code. No compiled firmware or OTA binary is provided here.

## Ozellikler / Features
- 433.775 MHz LoRa APRS iGate; APRS-IS TCP connectivity and optional RF digipeater
- GPS tracker: off / LoRa RF / APRS-IS / both; adjustable SmartBeaconing, status, separate callsign and APRS symbol
- Optional mobile iGate location via GPS; configurable fixed/mobile profiles
- APRS messaging over RF or APRS-IS, message history and ACK handling
- Opt-in, restricted APRS-IS-to-RF message bridge for recently heard RF callsigns
- BME280 APRS TLM: sensor presence and fresh/valid readings required; no telemetry with absent sensor
- Multiple remembered Wi-Fi networks for hotspot/mobile use
- Segmented web panel: iGate&Digi, Tracker, Mesajlar, Telemetri, Wi-Fi, Sistem and Yedek
- JSON backup/restore for additional configuration groups; OTA via web panel
- OLED SSD1306/SH1106 options

## Pinout (Heltec Wireless Stick Lite V3)
| Peripheral | ESP32-S3 |
|---|---|
| SX1262 SCK/MISO/MOSI/CS | 9 / 11 / 10 / 8 |
| SX1262 RST/DIO1/BUSY | 12 / 14 / 13 |
| OLED/BME280 SDA/SCL (Wire1) | 41 / 42 |
| GPS TX -> ESP32 RX | GPIO47 |
| GPS RX <- ESP32 TX | GPIO48 |
| GPS UART baud | 9600 |

Use **3.3 V compatible logic** and verify power requirements for each sensor/GNSS board.

## Build (Arduino IDE)
1. Install Espressif ESP32 core **3.3.8**, select **ESP32S3 Dev Module**, flash size **4 MB**, OTA-capable partition scheme.
2. Install RadioLib **7.8.1**, Adafruit GFX, Adafruit SSD1306, Adafruit SH110X, TinyGPSPlus, Adafruit BME280 and Adafruit Unified Sensor (including library dependencies).
3. No extra secrets header is needed. The default setup/admin passwords are `1234567890`; change them using the web panel immediately after first setup.
4. Open `TA1FM_LoRa_APRS_iGate_v2.0.0-beta.1.ino` in its Arduino sketch folder and compile.
5. Keep the previously tested firmware available for rollback. Test with RF functions disabled initially.

**Security:** The published default password (`1234567890`) is known to everyone. Change it on first setup and do not expose the administration interface to the public internet. APRS-IS passcodes and Wi-Fi credentials contained in JSON exports are plain text: keep backups private.

## Important limitations
- The source is provided for ongoing testing. No independent CI/Arduino compile or hardware verification has been performed for this GitHub publication.
- APRS-IS -> RF bridge remains opt-in, limited to recently heard stations, and subject to TX protection. Do not assume reliable delivery without field testing.
- RF sending is subject to minimum gap / per-hour limits in the firmware; review local amateur radio regulations and station authorization.
- Current `main` branch and its firmware/installer are NOT changed by this beta branch.

## Project
https://github.com/TA1FM/TA1FM-LoRa-APRS-iGate-Digipeater

73 de TA1FM!
