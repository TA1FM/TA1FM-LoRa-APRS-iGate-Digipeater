# TA1FM LoRa APRS — Installation & Usage Guide (English)

Updated **25 November 2026** for the user-confirmed working/stable firmware builds. **Hardware variants are not interchangeable.**

## Supported firmware

| Board | MCU / radio | Tested build | Archive |
|---|---|---|---|
| Heltec Wireless Stick Lite V3 | ESP32-S3 / SX1262 | V2.0.0 DEV8 **STABLE** | [DEV8](heltec/WSL_V3/DEV8/) |
| LILYGO T-Beam V1.2 SX127X | ESP32 / SX127X (tested: SX1278) | DEV9.4 **STABLE** | [DEV9.4](lilygo/TBeam_V1.2_SX127X/) |
| ESP32 DIY 30-pin NoLoRa | ESP32 WROOM / no LoRa | DEV8.3 **STABLE** | [DEV8.3](esp32diy/Esp32_NoLora/) |

"Stable" refers to the user's confirmed tested hardware and build. It is **not** proof that all revisions with the same chipset work.

## Browser-based USB installer

Open [TA1FM Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/) in Chrome or Edge. Choose the **exact board and firmware** from the grouped dropdown, connect a USB data cable, then click **INSTALL FIRMWARE** and select the correct serial port.

A suitable antenna must be installed on LoRa boards before any transmission. The installer uses each selected board's **merged image at offset `0x0`**. Installation may affect saved data; back up configuration first.

**Never flash an ESP32-S3 image to a classic ESP32, or an SX127X T-Beam image to an SX126x T-Beam.**

## File types and OTA

- **`.merged.bin`**: complete USB install / Web Flasher image.
- **`.bin`** or **`.ino.bin`**: application firmware for compatible OTA updates.
- **`.bootloader.bin`**, **`.partitions.bin`**: individual flashing components, for advanced use.
- **`.ino`**: Arduino source.

**Do not use a merged image for OTA.** OTA requires a compatible partition scheme and application binary for that same board.

## First-time Wi-Fi / administration

If no Wi-Fi network is configured, the setup AP may be started:

| Field | Default |
|---|---|
| Setup SSID | `TA1FM-APRS-SETUP` |
| AP password | `1234567890` |
| Setup page | `http://192.168.4.1` |
| Panel user | `admin` |
| Panel password | `1234567890` in the archived source builds; verify for your specific installation |

Change the default passwords after setup and do not expose the local admin interface directly to the Internet. Heltec DEV8 also offers a PRG/GPIO0 button workflow for OLED pages and AP mode.

## Features and operating modes

**RF-capable boards (Heltec, T-Beam):** LoRa APRS iGate, conditional backup Digipeater, APRS-IS, GPS Tracker with SmartBeaconing, APRS messaging and acknowledgements, RF/IS beacon options, web panel, OTA, OLED, configuration backup/restore, and available telemetry. The basic 433.775 MHz LoRa APRS modem settings used in this project are **SF12, BW125 kHz, CR 4/5**.

**ESP32 DIY NoLoRa:** GPS / Wi-Fi / APRS-IS Tracker and supported web/OLED functionality. **No RF receive, RF transmit, or RF Digipeater.**

The web panel includes status, iGate/Digi, Tracker, Messages, Telemetry, Wi-Fi, System/OTA and Backup sections, as supported by the board.

Battery measurements depend on hardware: Heltec ADC, T-Beam AXP2101; an ESP32 DIY board without a connected measurement circuit reports unavailable, not a fabricated value. Optional BME280 telemetry requires the actual sensor.

## Initial APRS configuration

1. Set **your own authorized callsign and SSID**.
2. Configure Wi-Fi and APRS-IS server / login/passcode (the project commonly uses `euro.aprs2.net:14580`).
3. Set correct coordinates, symbol and comments.
4. On RF boards, configure beacon, Digi and power only within legal/hardware limits, and confirm antenna connection.
5. Check APRS-IS login for `verified`, GPS FIX and received packets before enabling more features.

## Troubleshooting

- **No setup AP:** check boot serial log, Wi-Fi state and supported AP button behavior.
- **APRS-IS not verified:** check network, callsign, server, port, and passcode.
- **No GPS fix:** check GPS power, UART and antenna placement.
- **No RF packets:** check the board's specific radio profile, antenna and LoRa settings.
- **OTA failed:** verify application BIN vs merged image and partition compatibility.
- **Unexpected reset:** check the serial monitor (115200 baud) for the final successful initialization line.

## Documentation

[Turkish setup and usage](KURULUM.md) · [Arduino IDE build guide](DERLEME.md) · [Hardware pinouts](DONANIM_BAGLANTILARI.md) · [Release notes](RELEASE_NOTES/) · [Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/)

Source repository: https://github.com/TA1FM/TA1FM-LoRa-APRS-iGate-Digipeater

**73 de TA1FM!** 📡
