# TA1FM MultiBoard — Donanım Bağlantıları

**25 Kasım 2026** itibarıyla test edilen kartların bağlantı bilgileri. Yeni kart revizyonlarında üretici şemasını ve fiziksel PCB işaretlerini ayrıca doğrulayın. Bu tabloları başka T-Beam varyantlarına aynen uygulamayın.

## 1. Desteklenen cihazlar

| Kart | Mikrodenetleyici | LoRa | Güç / pil |
|---|---|---|---|
| Heltec Wireless Stick Lite V3 | ESP32-S3 | SX1262 | Pil ölçümü ADC |
| LILYGO T-Beam V1.2 | ESP32 | SX127X (test kartı SX1278) | **AXP2101** PMU |
| ESP32 DIY DevKit 30-pin | ESP32 WROOM | **Yok** | Harici ADC devresi yoksa **N/A** |

## 2. Heltec Wireless Stick Lite V3 (ESP32-S3 + SX1262)

| Bağlantı | ESP32-S3 GPIO |
|---|---|
| LoRa SCK / MISO / MOSI / CS | **9 / 11 / 10 / 8** |
| LoRa RST / DIO1 / BUSY | **12 / 14 / 13** |
| OLED ve BME280 I²C Wire1 SDA / SCL | **41 / 42** |
| GPS TX → ESP32 RX | **47** |
| GPS RX ← ESP32 TX | **48** |
| PRG / ekran-AP tuşu | **0** |
| Batarya ADC | **1** |
| Batarya ADC kontrol | **37** |
| GPS baud | **9600** |

Harici SSD1306/SH1106 OLED ve isteğe bağlı BME280, I²C üzerinden bağlanır. Kartın OLED seçimi ekran modeline göre yapılır. Li-ion pil ölçümü mevcut kartın ADC/devre oranına bağlıdır; gerekirse voltmetreyle kalibre edilmelidir.

## 3. LILYGO T-Beam V1.2 — SX127X / AXP2101

Aşağıdaki bağlantılar **kullanıcının Meshtastic logu ve SX127X tanı/RX/TX testleri** ile bu kart üzerinde doğrulanan/uyumlu bilgilerdir:

| İşlev | ESP32 GPIO |
|---|---|
| LoRa SCK / MISO / MOSI / CS | **5 / 19 / 27 / 18** |
| LoRa RST | **23** |
| SX127X DIO0 | **26** |
| SX127X anten anahtarı RXEN / TXEN | **14 / 13** |
| GPS TX → ESP32 RX | **34** |
| GPS RX ← ESP32 TX | **12** |
| OLED / PMU I²C SDA / SCL | **21 / 22** |
| T-Beam butonu | **38** (dahili pull-up yok) |
| GPS baud | **9600** |
| PMU | **AXP2101, I²C 0x34** |
| OLED test adresi | **0x3C** |

**Önemli:** ESP32 GPIO34–39 girişe özeldir ve dahili pull-up içermez; GPIO38 için `INPUT_PULLUP` kullanılmamalıdır. Bu T-Beam profilinde **SX126x BUSY/DIO1 pin şeması geçerli değildir**. SX1262/SX1268 modellerinin bağlantı, RF switch, TCXO ve PMU kontrolü farklı olabilir.

T-Beam DEV9.4 üzerinde web panelinden güç seçimi, LoRa RX ve TX ile gerçek APRS mesaj ACK alımı doğrulanmıştır. Donanımın gerçek RF çıkış gücü ayrıca wattmetreyle ölçülmemiştir.

## 4. ESP32 DIY 30-pin / NoLoRa

| İşlev | GPIO |
|---|---|
| GPS TX → ESP32 RX | **16** |
| GPS RX ← ESP32 TX | **17** |
| Harici OLED I²C SDA / SCL | **21 / 22** |
| BOOT düğmesi | **0** |
| LoRa modülü | **Yok** |
| Batarya ölçüm devresi | Kartta harici devre kurulmadıysa **yok** |

Bu kart yalnızca Wi-Fi/GPS üzerinden APRS-IS Tracker olarak kullanılır. GPS, OLED ve Wi-Fi özellikleri LoRa modülünün varlığına bağlı değildir. Pil voltajı ölçümü için güvenli gerilim bölücü/ADC donanımı gerekir; rastgele Li-ion pil gerilimi GPIO'ya doğrudan uygulanmaz.

LoRa modülü eklenen DIY modeller ileride **ayrı `esp32diy/Esp32_LoRa/`** kart profilinde ele alınacaktır.

## 5. Güç ve RF güvenliği

- Bağlantıları güç kapalıyken yapın. GPS ve OLED modüllerinin besleme gerilimini kartın gerçek şemasından doğrulayın.
- **LoRa yayını öncesi uygun anten takın.**
- SX127X ile SX126x sürücüleri, DIO interrupt pinleri ve RF anahtar kontrollerini birbirinin yerine kullanmayın.
- Pil/PMU ölçümü bulunmayan kartlarda **uydurma voltaj veya yüzde gösterilmemeli**.
- Gerçek çıkış gücü, anten kazancı, varsa harici PA ve amatör telsiz düzenlemelerini dikkate alın.

[Web Flasher](https://ta1fm.github.io/TA1FM-LoRa-APRS-iGate-Digipeater/) · [Kurulum/kullanım](KURULUM.md) · [Derleme](DERLEME.md)

**73 de TA1FM!** 📡
