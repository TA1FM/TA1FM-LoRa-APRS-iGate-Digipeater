# LILYGO T-Beam V1.2 — SX127X

Bu klasör TA1FM **iGate / Digipeater / Tracker** yazılımının LILYGO T-Beam V1.2, **SX127X** LoRa radyo ailesi için ayrılmış firmware arşividir.

## Donanım ve durum

- Kart: LILYGO T-Beam V1.2, ESP32
- Radyo ailesi: SX127X (kullanıcının kartı mağazada SX1278 olarak belirtilmiş; SX127x SPI kimlik kaydı ve RX/TX testi olumlu)
- Güç yönetimi: AXP2101
- GPS ve OLED: testte çalıştı
- Wi-Fi, APRS-IS, mesajlaşma ve web panel: testte çalıştı
- Web panelinden TX gücü seçimi: DEV9.4 üzerinde kullanıcı tarafından çalıştığı bildirildi
- Durum: **DEV9.4 STABLE — kullanıcı tarafından 25 Kasım 2026'da kararlı çalıştığı doğrulandı.** Bu doğrulama **test edilen T-Beam V1.2 SX127X kartı** içindir; bütün T-Beam revizyonlarında SX127X/SX126X desteği kanıtı değildir.

## Firmware dosyaları

Aşağıdaki kaynak kodu ve derlenmiş firmware dosyaları GitHub'a yüklenmiştir.

| Dosya | Kullanım |
|---|---|
| `TA1FM_iGate_Digi_Tracker_LILYGO T-Beam V1.2_SX127X.ino` | Arduino kaynak kodu (gerekli .h/variant dosyalarıyla birlikte saklanmalıdır) |
| `TA1FM_iGate_Digi_Tracker_LILYGO T-Beam V1.2_SX127X.bin` | Uygulama firmware'i; uygun OTA bölüm düzeniyle kullanılır |
| `TA1FM_iGate_Digi_Tracker_LILYGO T-Beam V1.2_SX127X.merged.bin` | USB üzerinden tam flash için birleşik firmware |
| `TA1FM_iGate_Digi_Tracker_LILYGO T-Beam V1.2_SX127X.bootloader.bin` | Bootloader |
| `TA1FM_iGate_Digi_Tracker_LILYGO T-Beam V1.2_SX127X.partitions.bin` | Flash bölüm tablosu |

**Uyarı:** `merged.bin` dosyasını OTA paneline yüklemeyin. Ayrı bootloader/partition dosyalarıyla yükleme yapılacaksa adresleri derleme yapılandırmasından doğrulayın. Bu firmware'i başka karta doğrudan yüklemeyin.

Bu klasör, `esp32diy/Esp32_NoLora/` dosyalarından ayrıdır.

## Kaynak koddan derleme notu

MultiBoard `.ino` dosyası `board_config.h` ve `variants/` yardımcı `.h` dosyalarını kullanır. Bu yardımcı dosyalar burada henüz yer almadığından hazır firmware indirmek mümkündür ancak `.ino` dosyası **tek başına eksiksiz bir derleme paketi değildir**.

## Belgeler

[Türkçe kurulum ve kullanım](../../KURULUM.md) · [Derleme](../../DERLEME.md) · [Donanım bağlantıları](../../DONANIM_BAGLANTILARI.md) · [STABLE sürüm notu](../../RELEASE_NOTES/lilygo-tbeam-v12-sx127x-dev94-stable.md)
