# TA1FM LoRa APRS iGate & Digi — Heltec WSL V3 / V2.0.0 DEV8 STABLE

**Önerilen Release etiketi:** `heltec-wsl-v3-dev8-stable`

## Sürüm açıklaması

24 Kasım 2026 tarihinde kullanıcı tarafından **kararlı çalıştığı doğrulanan Heltec Wireless Stick Lite V3** firmware sürümüdür. Kullanılan geliştirme adı **V2.0.0 DEV8** olup, DEV8.1 adlandırması kullanılmamaktadır.

### Donanım
- Heltec Wireless Stick Lite V3 (ESP32-S3)
- SX1262 LoRa, 433.775 MHz APRS ağı
- OLED, GPS ve fiziksel PRG butonu

### Özellikler
- LoRa APRS iGate ve Digipeater
- GPS Tracker ve SmartBeaconing
- APRS-IS bağlantısı, mesajlaşma, ACK
- Web paneli, OTA ve Wi-Fi ayarları
- OLED ekran sayfaları, buton ile ekran/AP kontrolü

### Dosyalar
- [Arduino kaynak kodu](../heltec/WSL_V3/DEV8/TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino)
- [Uygulama BIN (OTA için; uygun partition gerekli)](../heltec/WSL_V3/DEV8/TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.bin)
- [Birleşik BIN (USB tam kurulum)](../heltec/WSL_V3/DEV8/TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.merged.bin)
- [Bootloader](../heltec/WSL_V3/DEV8/TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.bootloader.bin)
- [Partition tablosu](../heltec/WSL_V3/DEV8/TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.partitions.bin)

**Dikkat:** Bu dosyalar yalnızca Heltec WSL V3 içindir. `merged.bin` dosyası OTA'ya yüklenmez. Yanlış karta yükleme yapmayın.

[Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/)
