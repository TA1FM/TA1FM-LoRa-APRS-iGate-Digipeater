# TA1FM LoRa APRS iGate — Heltec WSL V3 — V2.1.0

Heltec Wireless Stick Lite V3 (ESP32-S3 + SX1262) icin V2.1.0 Arduino kaynak kodu.

## Duzeltmeler
- Bos NVS/ilk fabrika kurulumunda Wi-Fi dogrudan `WIFI_AP` modunda baslar; STA -> AP_STA gecisi atlanir.
- Tracker kapaliyken (`trkmode=0`) yedekte saklanan Tracker STATUS tercihi (`tstena`, `tstmode`) geri yuklemeyi engellemez.
- Tracker konum takibi acikken farkli ve gecerli bir Tracker cagrı isareti kontrolu korunur.
- Firmware surum etiketi `V2.1.0` olarak guncellenmistir.
- Eski JSON yedeklerin format kimligi `TA1FM-V2.0-DEV7-settings` olarak korunur.

## Derlenmis dosyalar ve yukleme

- `TA1FM_LoRa_APRS_iGate_V2_1_0.bin` — OTA / uygulama firmware (1.264.672 bayt)
- `TA1FM_LoRa_APRS_iGate_V2_1_0.merged.bin` — tum flash goruntusu (4 MB), USB Web Flasher ile 0x0 adresinden yuklenir
- `TA1FM_LoRa_APRS_iGate_V2_1_0.bootloader.bin` — bootloader
- `TA1FM_LoRa_APRS_iGate_V2_1_0.partitions.bin` — bolum tablosu
- `boot_app0.bin` — OTA baslangic verisi

Kaynak kod ve derlenmis BIN dosyalari GitHub klasorune yuklenmistir. Web Flasher varsayilan Heltec V2.1.0 secenegini kullanir.

**Test sonucu (25 Kasim 2026):** Kullanici V2.1.0 BIN dosyasini USB Web Flasher ile basariyla yukledigini ve onceki ayar JSON yedegini sorunsuz geri yukledigini dogruladi. Bu sonuc test edilen Heltec WSL V3 donanimi icin gecerlidir.

**Uyari:** `merged.bin` yalnizca tam flash / USB yukleme icindir. OTA panelinden `.bin` uygulama dosyasini kullanin, `merged.bin` dosyasini OTA'ya yuklemeyin.
Kart: Heltec Wireless Stick Lite V3, ESP32-S3, SX1262. OTA destekli bolum duzeni gereklidir.

T-Beam ve ESP32 DIY firmware dosyalarina dokunulmamistir.
