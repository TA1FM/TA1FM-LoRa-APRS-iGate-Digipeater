# TA1FM LoRa APRS iGate & Digi
## Donanım Bağlantıları / Hardware Connections

### Desteklenen Donanım / Supported Hardware

- Heltec Wireless Stick Lite V3 (ESP32-S3 + SX1262)
- Harici SSD1306 OLED ekran (128×64, I²C, 3.3V uyumlu)

**Önemli:** Bu yazılım sürümü yalnızca Heltec Wireless Stick Lite V3 için hazırlanmıştır.

### OLED GPS BME SENSOR Bağlantı Tablosu

## Pinout (Heltec Wireless Stick Lite V3)
| Peripheral | ESP32-S3 |
|---|---|
| SX1262 SCK/MISO/MOSI/CS | 9 / 11 / 10 / 8 |
| SX1262 RST/DIO1/BUSY | 12 / 14 / 13 |
| OLED/BME280 SDA/SCL (Wire1) | 41 / 42 |
| GPS TX -> ESP32 RX | GPIO47 |
| GPS RX <- ESP32 TX | GPIO48 |
| GPS UART baud | 9600 |

### Bağlantı Şeması

```text
  HELTEC WIRELESS STICK LITE V3
  ┌──────────────────────────┐
  │                          │
  │  3.3V ─────────────────────── VCC  ┐
  │                          │         │
  │  GND  ─────────────────────── GND  │
  │                          │         │
  │  GPIO41 ───────────────────── SDA  │
  │                          │         │
  │  GPIO42 ───────────────────── SCL  │
  │                          │         │
  └──────────────────────────┘         │
                              ┌───────────────┐
                              │ SSD1306 OLED  │
                              │ 128×64 I²C    │
                              └───────────────┘
```

### Açıklamalar / Notes

**Türkçe**

- SX1262 LoRa radyo devresi Heltec kartının üzerinde bulunmaktadır.
- OLED ekran harici olarak bağlanmaktadır.
- OLED haberleşmesi I²C üzerinden GPIO41 ve GPIO42 ile yapılmaktadır.
- OLED modülünün 3.3V beslemeye uygun olduğundan emin olun.
- Bağlantıları cihazın enerjisi kesilmişken yapın.
- RF yayını yapmadan önce uygun anten bağlayın.

**English**

- The SX1262 LoRa transceiver is integrated into the Heltec board.
- An external SSD1306 OLED display is connected using I²C.
- SDA: GPIO41, SCL: GPIO42.
- Use a 3.3V-compatible OLED module.
- Disconnect power before wiring.
- Connect a suitable antenna before RF transmission.

---

**TA1FM LoRa APRS iGate & Digi**

73 de TA1FM!
