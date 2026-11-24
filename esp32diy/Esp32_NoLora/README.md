# ESP32 DIY — No LoRa (Wi-Fi APRS-IS Tracker)

Bu klasör, **ESP32 DevKit 30-pin / LoRa modülü olmayan** TA1FM Wi-Fi APRS-IS Tracker derlemesine ayrılmıştır.

- Kart: ESP32 DevKit 30-pin (ESP32 WROOM)
- LoRa: **YOK** — radyo üzerinden gönderim/alım yapılmaz
- GPS UART: RX GPIO16, TX GPIO17
- OLED I²C: SDA GPIO21, SCL GPIO22
- Durum: **DEV8.3 STABLE — kullanıcı tarafından 25 Kasım 2026'da kararlı çalıştığı doğrulandı.** Bu sürüm **LoRa modülsüz** DIY kart içindir.

## Firmware dosyaları

Aşağıdaki firmware dosyaları GitHub klasörüne yüklenmiştir:

| Dosya | Amaç |
|---|---|
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.bin` | Uygulama firmware'i; OTA yalnızca uygun bölüm düzeninde |
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.bootloader.bin` | Bootloader |
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.partitions.bin` | Bölüm tablosu |
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.merged.bin` | USB ile tam flash için birleşik görüntü |
| `TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.ino` | Arduino ana kaynak kodu |

**Dikkat:** `merged.bin` OTA üzerinden yüklenmez. Flash adresleri ve bölüm düzeni dosyalar doğrulanmadan varsayılmamalıdır.

LoRa modülü takılı ESP32 DIY sürümleri ileride ayrı `esp32diy/Esp32_LoRa/` klasörüne konacaktır. Bu klasörde yalnızca No LoRa dosyaları tutulur.

## Kaynak koddan derleme notu

Bu MultiBoard `.ino` kaynağı `board_config.h` ve kart/radyo `variants/` başlık dosyalarını gerektirir. Bu yardımcı `.h` dosyaları henüz GitHub klasörüne yüklenmedi; dolayısıyla `.ino` tek başına yeniden derlenebilir tam paket değildir.

## Belgeler

[Türkçe kurulum ve kullanım](../../KURULUM.md) · [Derleme](../../DERLEME.md) · [Donanım bağlantıları](../../DONANIM_BAGLANTILARI.md) · [STABLE sürüm notu](../../RELEASE_NOTES/esp32diy-nolora-dev83-stable.md)
