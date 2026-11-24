# TA1FM LoRa APRS iGate & Digi — Heltec WSL V3 DEV8

Bu klasör **Heltec Wireless Stick Lite V3 (ESP32-S3 + SX1262)** için **V2.0.0 DEV8 OLED / Buton / AP** yazılımının kaynak kodunu ve derlenmiş firmware dosyalarını içerir.

**Durum:** Kullanıcı tarafından çalışan referans olarak bildirilmiştir. Uzun süreli tüm saha testleri tamamlanmış bir STABLE etiketi değildir. DEV8.1 ayrı sürüm olarak kullanılmıyor.

## Dosyalar

| Dosya | Açıklama |
|---|---|
| `TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino` | Arduino kaynak kodu |
| `TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.bin` | Uygulama BIN; uygun OTA düzeninde |
| `TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.merged.bin` | **USB Web Flasher** tam firmware görüntüsü |
| `TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.bootloader.bin` | Ayrı bootloader |
| `TA1FM_LoRa_APRS_iGate_V2_0_0_DEV8_OLED_BUTON_AP.ino.partitions.bin` | Bölüm tablosu |

`.elf` hata ayıklama dosyası bilerek yüklenmemiştir.

## Özellikler

APRS-IS iGate, LoRa Digipeater, GPS Tracker ve SmartBeaconing, APRS mesajlaşma, web kontrol paneli, OLED ekran sayfaları, fiziksel tuşla ekran/AP kontrolü, OTA ve ayar yönetimi.

## Yükleme uyarısı

- **Sadece Heltec Wireless Stick Lite V3 / ESP32-S3 kartına yükleyin.** T-Beam ve klasik ESP32 DevKit yazılımları farklıdır.
- Web Flasher, `merged.bin` dosyasını flash adresi `0x0` üzerinden yükler; **OTA'ya `merged.bin` yüklemeyin.**
- USB yükleme mevcut yazılım ve ayarları etkileyebilir; önceden yedek alın.
- LoRa kullanımında doğru frekans bandı için uygun anten bağlayın.

[TA1FM Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/)
