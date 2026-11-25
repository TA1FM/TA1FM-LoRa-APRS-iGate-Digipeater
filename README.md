# TA1FM LoRa APRS iGate · Digipeater · Tracker

**TA1FM** amatör telsiz APRS uygulaması, farklı ESP32 kartlarında LoRa APRS iGate, RF Digipeater, GPS Tracker ve **LoRa modülü olmayan kartlarda Wi-Fi üzerinden APRS-IS Tracker** işlevlerini destekleyen bir yazılım ailesidir. Proje, Heltec Wireless Stick Lite V3 üzerinde başlamış, LILYGO T-Beam V1.2 SX127X ve ESP32 DIY NoLoRa kartlarına genişletilmiştir.

[🚀 **Web Flasher — USB ile firmware yükle**](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/) · [📦 Firmware dosyaları](https://github.com/TA1FM/TA1FM-LoRa-APRS-iGate-Digipeater) · [🏷️ Releases](https://github.com/TA1FM/TA1FM-LoRa-APRS-iGate-Digipeater/releases)

> **Güncelleme — 25 Kasım 2026:** Aşağıdaki üç kartın belirtilen firmware sürümleri kullanıcı tarafından çalışır ve kararlı olarak doğrulanmıştır. Bu doğrulama **yalnızca test edilen kart ve donanım yapılandırmaları** içindir; aynı işlemciyi kullanan her kartın otomatik desteklendiği anlamına gelmez.

**Heltec uyarısı (25 Kasım 2026):** Eski V2.0.0 DEV8 `merged.bin` dosyası boş NVS / fabrika ilk kurulumunda yeniden başlama hatası oluşturabildiği için Web Flasher üzerinden seçimi geçici olarak kapatılmıştır. Test edilen V2.1.0 düzeltmesinin kaynak kodu `heltec/WSL_V3/V2.1.0` klasörüne eklenmiştir. Yedek geri yükleme doğrulama düzeltmesi de bu kaynakta yer alır; yeni derlenmiş ve test edilmiş BIN henüz yayımlanmamıştır. T-Beam ve DIY firmware yükleme seçenekleri korunmuştur.

## Desteklenen kartlar ve son çalışan sürümler

| Kart | Donanım | Doğrulanan sürüm | Firmware klasörü |
|---|---|---|---|
| **Heltec Wireless Stick Lite V3** | ESP32-S3 + SX1262 | **V2.1.0 kaynak kodu — yeni BIN bekleniyor** | [heltec/WSL_V3/V2.1.0](heltec/WSL_V3/V2.1.0/) |
| **LILYGO T-Beam V1.2 SX127X** | ESP32 + SX127X (test cihazı SX1278), AXP2101 | **DEV9.4 — STABLE** | [lilygo/TBeam_V1.2_SX127X](lilygo/TBeam_V1.2_SX127X/) |
| **ESP32 DIY / NoLoRa** | ESP32 WROOM DevKit 30 pin, **LoRa yok** | **DEV8.3 — STABLE** | [esp32diy/Esp32_NoLora](esp32diy/Esp32_NoLora/) |

**Önemli:** Her kartın **kendine ait firmware dosyasını** seçin. Heltec'in ESP32-S3 firmware'i klasik ESP32 kartlara; T-Beam'in SX127X firmware'i SX126x T-Beam kartlara yüklenmemelidir.

## Yeni nesil özellikler

- **LoRa APRS iGate:** RF üzerinden alınan APRS paketlerini APRS-IS ağına aktarma.
- **RF Digipeater:** Yapılandırmaya bağlı APRS paket tekrarlama ve internet yokken yedek çalışma.
- **GPS Tracker:** Sabit periyot veya **SmartBeaconing**; kartın yeteneklerine göre LoRa RF, APRS-IS veya her iki kanaldan konum gönderimi.
- **APRS mesajlaşma:** Mesaj gönderme/alma, ACK takibi ve ilgili kanal ayarları.
- **Gelişmiş web paneli:** Ana durum, iGate/Digi, Tracker, Mesajlar, Telemetri, Wi-Fi, Sistem ve Yedekleme bölümleri.
- **APRS telemetri:** Desteklenen ölçüm kaynaklarından veri; bulunmayan sensörün değerini üretmeme.
- **OLED:** Desteklenen SSD1306/SH1106 ekran seçenekleri; Heltec DEV8'de sayfalar arası geçiş ve PRG tuşuyla AP kontrolü.
- **OTA firmware güncelleme**, ayar yedekleme/geri yükleme ve kayıtlı Wi-Fi bağlantılarının yönetimi.
- **Kart bazlı pil bilgisi:** Heltec ADC, T-Beam AXP2101; harici ölçüm devresi bulunmayan ESP32 DIY için voltaj yoksa **N/A**.

**NoLoRa farkı:** ESP32 DIY NoLoRa sürümünde **LoRa RF TX/RX, RF iGate ve RF Digi yoktur**. GPS/Wi-Fi üzerinden APRS-IS Tracker, web panel ve desteklenen ekran özellikleri kullanılır.

**LoRa yapılandırması:** Türkiye'deki 433.775 MHz APRS kullanımı için projede SF12, BW125 kHz, CR 4/5 ayarları esas alınır. Doğru anten ve geçerli amatör telsiz yetkisi gereklidir. RF gücü kartın çıkış katına ve mevzuata uygun tutulmalıdır.

## Web Flasher ile USB kurulumu

[**TA1FM Web Flasher'a git**](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/)

1. Chrome veya Edge ile Web Flasher'ı açın.
2. **Açılır menüden kartın tam modelini ve sürümünü** seçin.
3. Uygun USB veri kablosunu bağlayın. LoRa kullanan kartta doğru anten takılı olsun.
4. **INSTALL FIRMWARE** düğmesiyle COM portunu ve yüklemeyi onaylayın.
5. İlk açılışta Wi-Fi/APRS ayarlarını yapılandırın.

Web Flasher, seçilen kart için ilgili **`merged.bin`** görüntüsünü **0x0** ofsetiyle yükler. Mevcut ayarlar silinebilir; yüklemeden önce yedek alın. **`merged.bin` dosyasını OTA paneline vermeyin.** OTA için aynı kartın uygun bölümleme düzeniyle üretilmiş **uygulama `.bin`** dosyası kullanılır.

## İlk kurulum ve web paneli

Kaydedilmiş Wi-Fi yoksa kurulum AP'si oluşturulur:

| Alan | Varsayılan |
|---|---|
| Wi-Fi adı | `TA1FM-APRS-SETUP` |
| AP parolası | `1234567890` |
| Kurulum IP | `http://192.168.4.1` |
| Panel kullanıcısı | `admin` |
| Panel parolası | Bu üç kaynak sürümde `1234567890` (yükleyeceğiniz koddan kontrol edin) |

Şifreleri ilk kurulumdan sonra değiştirin, yönetim panelini doğrudan internete açmayın. Kendi amatör telsiz çağrı işaretinizi, SSID'nizi, APRS-IS giriş bilgilerini ve koordinatlarınızı kullanın.

## Kaynak dosyaları ve sürüm yönetimi

Çalışan kart sürümlerinin **`.ino`, uygulama `.bin`, birleşik `.merged.bin`, `.bootloader.bin` ve `.partitions.bin`** dosyaları yukarıdaki kart klasörlerindedir. Eski V1.5.7.1, V1.5.8 ve V2.0.0 BETA 1 (DEV7.2) sürümleri geri dönüş / geçmiş referansı olarak korunmaktadır.

**Derleme notu:** T-Beam DEV9.4 ve ESP32 DIY NoLoRa DEV8.3 için `board_config.h` ve `variants/boards/`, `variants/radios/` başlık dosyaları ilgili GitHub klasörlerine yüklenmiştir. Kaynak koddan derlerken **tüm ilgili klasörü** indirin; yalnız `.ino` dosyası yeterli değildir. `board_config.h` içinde doğru kart/radyo seçili olmalıdır. Hazır BIN dosyaları da mevcuttur. Tüm model çeşitlerinin otomatik algılanması henüz doğrulanmamıştır.

**Release bilgisi:** Kartlara özel STABLE sürüm notları [RELEASE_NOTES](RELEASE_NOTES/) klasöründedir. GitHub Release sayfasındaki yayımlanma durumu ayrıca kontrol edilmelidir; sürüm notu dosyasının bulunması tek başına yayımlanmış Release anlamına gelmez.

## Dokümantasyon

- [📘 Türkçe kurulum ve kullanım](KURULUM.md)
- [🛠️ Arduino IDE derleme](DERLEME.md)
- [🔌 Donanım bağlantıları](DONANIM_BAGLANTILARI.md)
- [🌍 English installation guide](INSTALLATION_EN.md)
- [📋 STABLE sürüm notları](RELEASE_NOTES/)

## Ekran görüntüleri

![TA1FM Web Panel](images/webpanel1.png)
![APRS Mesajlar](images/webpanel2.png)
![Heltec OLED / GPS / BME bağlantıları](images/oled-gps-bme-baglanti.png)

Proje: **TA1FM — Türkiye** · Lisans: **GPL-3.0** (depo içindeki LICENSE dosyası geçerlidir).

**73 de TA1FM!** 📡
