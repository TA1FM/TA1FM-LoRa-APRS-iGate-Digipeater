# ESP32 DIY — No LoRa (Wi-Fi APRS-IS Tracker)

Bu klasör, **ESP32 DevKit 30-pin / LoRa modülü olmayan** TA1FM Wi-Fi APRS-IS Tracker derlemesine ayrılmıştır.

- Kart: ESP32 DevKit 30-pin (ESP32 WROOM)
- LoRa: **YOK** — radyo üzerinden gönderim/alım yapılmaz
- GPS UART: RX GPIO16, TX GPIO17
- OLED I²C: SDA GPIO21, SCL GPIO22
- Referans: **DEV8.3 BOOT DIAG TEST** (Wi-Fi / web panel açılışı test edildi; uzun süreli STABLE onayı yok)

## Firmware dosyaları

Aşağıdaki dört dosyanın adları kullanıcı tarafından bildirildi; **henüz bu klasöre yüklenmediler**:

| Dosya | Amaç |
|---|---|
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.bin` | Uygulama firmware'i; OTA yalnızca uygun bölüm düzeninde |
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.bootloader.bin` | Bootloader |
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.partitions.bin` | Bölüm tablosu |
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.merged.bin` | USB ile tam flash için birleşik görüntü |

**Dikkat:** `merged.bin` OTA üzerinden yüklenmez. Flash adresleri ve bölüm düzeni dosyalar doğrulanmadan varsayılmamalıdır.

LoRa modülü takılı ESP32 DIY sürümleri ileride ayrı `esp32diy/Esp32_LoRa/` klasörüne konacaktır. Bu klasörde yalnızca No LoRa dosyaları tutulur.
