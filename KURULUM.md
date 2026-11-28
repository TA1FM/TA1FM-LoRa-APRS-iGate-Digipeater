# TA1FM APRS — Türkçe Kurulum ve Kullanım Rehberi

Bu rehber **25 Kasım 2026 itibarıyla doğrulanan** Heltec DEV8, T-Beam DEV9.4 ve ESP32 DIY NoLoRa DEV8.3 sürümleri içindir.

## 1. Doğru kart / firmware seçimi

| Kart | İşlemci / radyo | Seçilecek Web Flasher sürümü |
|---|---|---|
| Heltec Wireless Stick Lite V3 | ESP32-S3 / SX1262 | **V2.0.0 DEV8** |
| LILYGO T-Beam V1.2 SX127X | ESP32 / SX127X (test kartı SX1278) | **DEV9.4** |
| ESP32 DIY 30-pin NoLoRa | ESP32 WROOM / radyo yok | **DEV8.3** |

Desteklenen test donanımının dışındaki bir karta firmware yüklemeyin. Özellikle **SX127X T-Beam** firmware'i **SX1262/SX1268** modellerine uygun değildir. ESP32 DIY NoLoRa cihazı RF APRS yayın/alımı yapmaz.

Kart firmware arşivleri: [Heltec](heltec/WSL_V3/DEV8/) · [T-Beam](lilygo/TBeam_V1.2_SX127X/) · [ESP32 DIY](esp32diy/Esp32_NoLora/).

## 2. Tarayıcıdan USB ile yükleme

[**TA1FM Web Flasher**](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/) açılır menüde kartları gruplandırır.

1. Bilgisayarda Chrome veya Edge kullanın.
2. Kartın tam modelini ve firmware sürümünü seçin.
3. USB veri kablosunu takın; LoRa kartlarında anteni takmadan RF yayınını başlatmayın.
4. **INSTALL FIRMWARE** düğmesine basıp COM portunu seçin.
5. Yüklemeyi onaylayıp cihazı yeniden başlatın.

Web Flasher, karta ait **`merged.bin`** dosyasını `0x0` adresinden yükler. Bu işlem ayarları etkileyebilir. Önceden **Yedek** bölümünden yapılandırma yedeği alın.

## 3. Manuel USB yükleme ve BIN türleri

| Dosya | Amaç |
|---|---|
| `.merged.bin` | Birleşik ilk kurulum / USB tam flash (`0x0`) |
| `.bin` / `.ino.bin` | Uygulama firmware'i; uyumlu OTA düzeninde |
| `.bootloader.bin` | Ayrı bootloader |
| `.partitions.bin` | Ayrı bölümleme tablosu |
| `.ino` | Arduino kaynak kodu |

Başka aracın kullanıldığı ayrı parçalı flash işlemlerinde adresleri tahmin etmeyin; derleme ayarlarını doğrulayın. **OTA'ya `merged.bin` yüklemeyin.** ESP32-S3 ve klasik ESP32 görüntülerini karıştırmayın.

## 4. İlk Wi-Fi / AP kurulumu

Kayıtlı Wi-Fi yoksa cihaz `TA1FM-APRS-SETUP` kurulum ağı açabilir.

- Kurulum AP şifresi: `1234567890`
- Kurulum paneli: `http://192.168.4.1`
- Panel kullanıcısı: `admin`
- Bu üç kaynaktaki varsayılan panel şifresi: `1234567890`

Cihazın erişim noktasına bağlanıp ağ listesinden ev Wi-Fi'nizi seçin. Normal STA bağlantısı kurulduğunda cihazın IP adresi üzerinden web panelini açın. Heltec DEV8'in **PRG/GPIO0** tuşuna uzun basarak AP moduna geçiş özelliği bulunur. AP davranışı / buton donanımı diğer kartlarda aynı olmak zorunda değildir.

İlk açılıştan sonra varsayılan şifreleri değiştirin; paneli internete doğrudan yayınlamayın.

## 5. Web paneli ve kullanım

Web paneli sürüme / kartın özelliklerine göre şu bölümleri içerir:

| Bölüm | Kullanım |
|---|---|
| **Ana** | Wi-Fi, IP, APRS-IS, LoRa, GPS ve son alınan paketler |
| **iGate & Digi** | APRS-IS yönlendirme, otomatik/yedek Digipeater, beacon ve WIDE yolları |
| **Tracker** | Çağrı işareti, konum, sembol, SmartBeaconing, sabit/gelişmiş gönderim |
| **Mesajlar** | APRS mesajı gönderme/alma, bekleyen ACK ve mesaj geçmişi |
| **Telemetri** | Pil ve mevcut sensör ölçümlerini kontrol etme |
| **Wi-Fi** | Ağ kayıtları ve bağlantı seçenekleri |
| **Sistem** | Cihaz durumu, yeniden başlatma, OTA |
| **Yedek** | Ayarları dışa aktarma ve geri yükleme |

**Önce çağrı işaretinizi, SSID'nizi ve APRS-IS passcode'unuzu yapılandırın.** Örnek çağrı işaretleri / koordinatları olduğu gibi kullanmayın.

## 6. APRS-IS iGate / Digi

433.775 MHz LoRa APRS için projede kullanılan temel modem değerleri:

| Parametre | Değer |
|---|---|
| Frekans | 433.775 MHz |
| Spreading factor | SF12 |
| Bant genişliği | 125 kHz |
| Kodlama oranı | 4/5 |

Varsayılan APRS-IS sunucu ayarı `euro.aprs2.net:14580` olabilir; paneldeki mevcut ayarı kontrol edin. APRS-IS bağlantı durumunda `verified` doğrulamasını arayın. Digi özelliğini yalnız uygun yetki, çağrı işareti, anten ve RF ayarlarıyla etkinleştirin. LoRa **olmayan DIY kartta iGate RF RX / Digi kullanılamaz**.

## 7. GPS Tracker ve SmartBeaconing

- **LoRa RF / APRS-IS / her iki kanal** seçeneği yalnız radyo donanımı bulunan sürümlerde anlamlıdır.
- **DIY NoLoRa:** Konum için GPS + Wi-Fi üzerinden **APRS-IS** kanalını kullanın.
- GPS FIX ve uydu durumunu web panelinden veya uygun OLED ekranından kontrol edin.
- Beacon periyodu, hareket/hız ayarları, APRS sembolü, yorum ve durum bilgisini panelden seçin.
- SmartBeaconing, GPS'in verdiği hız/hareket bilgilerine göre gönderim aralığını değiştirebilir.

## 8. APRS mesajlaşma

Mesajlar ekranından hedef çağrı işaretini ve metni girerek uygun kanaldan gönderim yapın. ACK alanında alınan onaylar görüntülenebilir. T-Beam DEV9.4'te SX127X üzerinden RF mesaj gönderimi ve **karşı istasyondan gerçek ACK alımı** doğrulandı. APRS-IS üzerinden mesaj göndermek için sunucu bağlantısı gerekir.

## 9. OLED, pil ve sensörler

- **Heltec DEV8:** OLED ekran sayfaları; kısa PRG tuş basımıyla ekran dolaşma, uzun basımla AP modu. Harici ekran SSD1306/SH1106 seçeneğine göre yapılandırılır.
- **T-Beam DEV9.4:** SSD1306 OLED ve GPS çalıştığı doğrulandı; pil/güç yönetimi AXP2101.
- **ESP32 DIY:** Harici OLED I²C ve GPS UART, pil ölçümü için harici uygun devre yoksa **N/A**.

Pil yüzdesi ölçüm yöntemine bağlı yaklaşık değerdir. BME280 sensörü yoksa uydurma sıcaklık/nem/basınç verisi gönderilmemelidir. RF güç ayarları kartın donanım limitlerine bağlıdır; SX127X ve SX1262 sınırlarını eşit varsaymayın.

## 10. OTA güncelleme

1. Firmware'i **aynı kart ve uyumlu bölümleme düzeni** için seçin.
2. Ayarlarınızı yedekleyin.
3. Web panelinin Sistem / OTA bölümünü açın.
4. Sadece **uygulama `.bin`** dosyasını yükleyin.
5. Cihaz yeniden başladıktan sonra sürümü ve ayarları kontrol edin.

**`.merged.bin` OTA dosyası değildir.** OTA başarısızsa bölümleme şemasını, dosya türünü ve cihazın mevcut firmware'ini kontrol edin.

## 11. Sorun giderme

| Belirti | Kontrol |
|---|---|
| Kurulum AP'si yok | Seri monitör 115200 baud; Wi-Fi ayarı, başlatma logu, varsa AP tuşu |
| APRS-IS verified olmuyor | Wi-Fi, çağrı işareti, APRS-IS sunucu / port / passcode |
| GPS FIX yok | GPS anteni, açık gökyüzü, UART ve güç hattı |
| LoRa RX yok | Doğru kart, anten, modem parametreleri ve LoRa başlatma logları |
| RF TX yok | Yetkilendirme/ayar, mesaj beacon izni, RF çıkış sınırı, anten |
| Watchdog reset | Seri logda son işlem; kart profilini ve güç yönetimini kontrol edin |
| `Incorrect size of core dump image` | Eski/uyumsuz coredump verisi olabilir; tek başına LoRa arızası kanıtı değildir |
| OTA reddedildi | Yanlış `merged.bin`, partition uyumsuzluğu, yanlış kart firmware'i |

## 12. Diğer rehberler

[Donanım bağlantıları](DONANIM_BAGLANTILARI.md) · [Arduino IDE derleme](DERLEME.md) · [English guide](INSTALLATION_EN.md) · [STABLE sürüm notları](RELEASE_NOTES/)

**73 de TA1FM!** 📡
