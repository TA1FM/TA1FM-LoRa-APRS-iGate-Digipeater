# TA1FM MultiBoard — Arduino IDE Derleme Rehberi

**Son güncelleme: 25 Kasım 2026.** Bu rehber kartlara özel son çalışan firmware sürümleri içindir.

[🚀 Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/) kullanıldığında **Arduino IDE ile derleme gerekmez**. Aşağıdaki adımlar kaynak koddan derlemek isteyenler içindir.

## 1. Kart seçimi

| Firmware | Arduino IDE Board | İşlemci | Radyo |
|---|---|---|---|
| [Heltec V2.0.0 DEV8](heltec/WSL_V3/DEV8/) | **ESP32S3 Dev Module** | ESP32-S3 | SX1262 |
| [LILYGO T-Beam V1.2 DEV9.4](lilygo/TBeam_V1.2_SX127X/) | **ESP32 Dev Module** | ESP32 | SX127X (test edilen SX1278) |
| [ESP32 DIY NoLoRa DEV8.3](esp32diy/Esp32_NoLora/) | **ESP32 Dev Module** | ESP32 | Yok |

**ESP32 ile ESP32-S3 için derlemeler farklıdır.** Birinin BIN dosyasını diğerine yüklemeyin. Referans derlemelerde flash boyutu **4 MB**; OTA gereken yapılandırmalarda **OTA bölümü içeren** partition düzeni kullanılmalıdır. Önceki başarılı derlemedeki flash ve partition ayarlarına bağlı kalın.

## 2. Gerekli yazılımlar ve kütüphaneler

Geliştirmede kullanılan ortam:

- **Arduino IDE 2.3.8**
- **Espressif ESP32 Arduino Core 3.3.8**
- **RadioLib 7.8.1**
- **Adafruit GFX, Adafruit SSD1306, Adafruit SH110X**
- **TinyGPSPlus**
- **Adafruit BME280 Library** ve **Adafruit Unified Sensor**
- **XPowersLib 0.3.3** — AXP2101'li T-Beam sürümü için
- ESP32 çekirdeğinin **SPI, Wire, WiFi, WebServer, Preferences ve Update** kütüphaneleri

Sürüm değişiklikleri derleme veya kart davranışını etkileyebilir.

## 3. Kaynak kod düzeni / önemli eksiklik

- **Heltec DEV8** kaynak kodu [kart klasöründe](heltec/WSL_V3/DEV8/) bağımsız `.ino` olarak yer alır.
- **T-Beam DEV9.4** ve **DIY NoLoRa DEV8.3** sürümlerinin kaynak kodları `#include "board_config.h"` kullanır.
- Bu MultiBoard kaynakları için **`board_config.h`** ve **`variants/boards/`, `variants/radios/`** altındaki ilgili `.h` dosyaları gerekir.
- **Bu yardımcı .h dosyaları GitHub'daki kart firmware klasörlerine henüz yüklenmedi.** Bu nedenle yalnızca GitHub'daki `.ino` dosyasını kopyalamak bu iki kartta derleme için yeterli olmayabilir.
- Eski çalışan kaynak klasörünüzdeki **eşleşen `.h` dosyalarını** aynı Arduino sketch dizininde saklayın. Farklı sürümlerin `.h` dosyalarını rastgele karıştırmayın.
- Gelecekte tek ana yazılım ve seçimle çoklu kart desteği hedeflenmektedir; tüm kartlarda tam otomatik algılama doğrulanmış değildir.

## 4. Derleme ve yükleme

1. Doğru kartın kaynak kodunu ve gerekiyorsa **aynı paket içindeki** yardımcı `.h` dosyalarını seçin.
2. Arduino IDE'de `.ino` dosyasını açın.
3. **Tools → Board** altından tablodaki işlemci kartını seçin.
4. Flash boyutunu **4 MB**, partition düzenini kullandığınız test edilmiş derlemeye uygun şekilde belirleyin.
5. Gerekli kütüphaneleri kurun. T-Beam için **XPowersLib**'i unutmayın.
6. **Verify / Derle** işlemini yapın; hata varsa ilk hata satırını inceleyin.
7. Doğru COM portu üzerinden karta yükleyin.
8. Seri Monitör: **115200 baud**. Başlangıçta LoRa, OLED, GPS, Wi-Fi ve web sunucusu kayıtlarını kontrol edin.

T-Beam SX127X için RadioLib 7.8.1 ile kesme callback tanımı gerektiğinde `radio.setDio0Action(onRadioRx, RISING);` şeklindedir.

## 5. Derlenmiş BIN dosyaları

Arduino IDE'nin **Sketch → Export Compiled Binary** menüsünden `.ino.bin`, `.ino.merged.bin`, `.ino.bootloader.bin`, `.ino.partitions.bin` dosyaları üretilebilir.

| Dosya | Kullanım |
|---|---|
| Uygulama `.bin` | Uyumlu OTA panelinden güncelleme |
| `.merged.bin` | USB ile tam kurulum; Web Flasher bunu **0x0** adresinden kullanır |
| `.bootloader.bin` ve `.partitions.bin` | Ayrı parçalı yükleme, adreslerin ayrıca doğrulanması gerekir |
| `.elf` | Hata ayıklama, cihaz yüklemesi veya Web Flasher için gerekmez |

**`merged.bin` OTA'ya yüklenmez.** Kartlar arasında firmware dosyalarını karıştırmayın.

## 6. Kaynak ve lisans

[GitHub ana sayfa](README.md) · [Donanım bağlantıları](DONANIM_BAGLANTILARI.md) · [Türkçe kullanım](KURULUM.md)

GPL-3.0 — depo içindeki [LICENSE](LICENSE) geçerlidir.

**73 de TA1FM!** 📡
