# TA1FM LoRa APRS iGate — Heltec WSL V3 — V2.1.0

Heltec Wireless Stick Lite V3 (ESP32-S3 + SX1262) icin V2.1.0 Arduino kaynak kodu.

## Duzeltmeler
- Bos NVS/ilk fabrika kurulumunda Wi-Fi dogrudan `WIFI_AP` modunda baslar; STA -> AP_STA gecisi atlanir.
- Tracker kapaliyken (`trkmode=0`) yedekte saklanan Tracker STATUS tercihi (`tstena`, `tstmode`) geri yuklemeyi engellemez.
- Tracker konum takibi acikken farkli ve gecerli bir Tracker cagrı isareti kontrolu korunur.
- Firmware surum etiketi `V2.1.0` olarak guncellenmistir.
- Eski JSON yedeklerin format kimligi `TA1FM-V2.0-DEV7-settings` olarak korunur.

## Durum ve yukleme
**Kaynak kodu mevcut; yeni V2.1.0 `.bin` ve `merged.bin` henuz derlenip bu klasore eklenmedi.**
Bu kaynak kodundaki son yedekleme kontrolu degisikligi henuz fiziksel cihazda test edilmemistir.

Arduino IDE ile derleme gerekleri: ESP32 core 3.3.8, RadioLib 7.8.1, TinyGPSPlus, Adafruit SSD1306/SH110X/GFX, Adafruit BME280.
Kart: ESP32S3 Dev Module, flash 4MB, OTA destekli bolum plani.

**Eski DEV8 merged.bin fabrika baslangicinda hata olusturabileceginden Web Flasher Heltec DEV8 secenegi gecici olarak kapatilmis durumdadir.**
Yeni BIN'i yayinlamadan once fabrika temiz ilk acilis, kayitli Wi-Fi ile yeniden acilis ve eski JSON yedek geri yukleme testleri yapilmalidir.

T-Beam ve ESP32 DIY firmware dosyalarina dokunulmamistir.
