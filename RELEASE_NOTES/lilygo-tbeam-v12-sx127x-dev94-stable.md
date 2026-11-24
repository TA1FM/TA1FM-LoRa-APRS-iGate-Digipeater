# TA1FM LoRa APRS iGate / Digi / Tracker — LILYGO T-Beam V1.2 SX127X DEV9.4 STABLE

**Önerilen Release etiketi:** `lilygo-tbeam-v12-sx127x-dev9.4-stable`

## Sürüm açıklaması

24 Kasım 2026 tarihinde kullanıcı tarafından **kararlı çalıştığı doğrulanan T-Beam V1.2 SX127X** firmware sürümüdür. Test edilen kart SX1278 olarak satılmış ve SX127x radyo kimliğiyle doğrulanmıştır.

### Donanım
- LILYGO T-Beam V1.2 (klasik ESP32)
- SX127X LoRa (test edilen çip SX1278)
- AXP2101 güç yönetimi, GPS, OLED

### Doğrulanan özellikler
- LoRa RF RX/TX ve karşı istasyondan APRS mesaj ACK yanıtı
- APRS-IS bağlantısı ve mesajlaşma
- Wi-Fi ve web paneli
- GPS ve OLED
- Web panelinden TX gücü seçimi (donanım sınırlarına uygun)

### Dosyalar
- [Arduino kaynak kodu](../lilygo/TBeam_V1.2_SX127X/TA1FM_iGate_Digi_Tracker_LILYGO%20T-Beam%20V1.2_SX127X.ino)
- [Uygulama BIN](../lilygo/TBeam_V1.2_SX127X/TA1FM_iGate_Digi_Tracker_LILYGO%20T-Beam%20V1.2_SX127X.bin)
- [Birleşik BIN](../lilygo/TBeam_V1.2_SX127X/TA1FM_iGate_Digi_Tracker_LILYGO%20T-Beam%20V1.2_SX127X.merged.bin)
- [Bootloader](../lilygo/TBeam_V1.2_SX127X/TA1FM_iGate_Digi_Tracker_LILYGO%20T-Beam%20V1.2_SX127X.bootloader.bin)
- [Partition tablosu](../lilygo/TBeam_V1.2_SX127X/TA1FM_iGate_Digi_Tracker_LILYGO%20T-Beam%20V1.2_SX127X.partitions.bin)

**Dikkat:** SX127X adı tüm T-Beam donanım revizyonlarının doğrulandığı anlamına gelmez. SX126x cihazına bu firmware yüklenmemelidir. `merged.bin` OTA için kullanılmaz. Kaynak kodun derlenmesi için ilgili board/radio .h variant dosyaları da gereklidir.

[Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/)
