# TA1FM Wi-Fi APRS-IS Tracker — ESP32 DIY NoLoRa DEV8.3 STABLE

**Önerilen Release etiketi:** `esp32diy-nolora-dev8.3-stable`

## Sürüm açıklaması

24 Kasım 2026 tarihinde kullanıcı tarafından **kararlı çalıştığı doğrulanan ESP32 DIY 30 pin / LoRa modülsüz** firmware sürümüdür. Referans geliştirme sürümü **DEV8.3**.

### Donanım
- ESP32 DevKit 30 pin (ESP32 WROOM)
- **LoRa modülü yok**; RF TX/RX desteklenmez
- GPS UART RX GPIO16 / TX GPIO17
- OLED I²C SDA GPIO21 / SCL GPIO22

### Özellikler
- Wi-Fi üzerinden APRS-IS Tracker
- GPS tabanlı konum
- Web panelinden ayarlar ve kontrol
- OLED ekran
- LoRa bulunmasa da çalışmaya devam eden yazılım

### Dosyalar
- [Arduino kaynak kodu](../esp32diy/Esp32_NoLora/TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.ino)
- [Uygulama BIN](../esp32diy/Esp32_NoLora/TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.bin)
- [Birleşik BIN](../esp32diy/Esp32_NoLora/TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.merged.bin)
- [Bootloader](../esp32diy/Esp32_NoLora/TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.bootloader.bin)
- [Partition tablosu](../esp32diy/Esp32_NoLora/TA1FM_LoRa_APRS_Esp32_Diy_Wifi_Tracker.partitions.bin)

**Dikkat:** Bu sürüm LoRa modülü olmayan kart içindir, RF APRS iGate veya Digipeater değildir. `merged.bin` OTA'ya yüklenmez. Kaynak kodu yeniden derlemek için gerekli board/radio .h dosyalarını da saklayın.

[Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/)
