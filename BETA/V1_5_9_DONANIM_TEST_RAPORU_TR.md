# TA1FM LoRa APRS iGate & Digipeater — V1.5.9 BETA Donanım Test Raporu

**Test tarihi:** 17 Kasım 2026  
**Durum:** Belirtilen test senaryoları PASS; tüm APRS protokolüne tam uyumluluk veya STABLE yayımlama onayı değildir.  
**Firmware:** `BETA/TA1FM_LoRa_APRS_iGate_V1_5_9_BETA.ino`  
**Önceki kararlı sürüm:** V1.5.8 STABLE (değiştirilmedi).

## Test düzeneği

- **iGate / Digipeater:** TA1FM-10; Heltec Wireless Stick Lite V3 (ESP32-S3, SX1262).
- **RF test vericisi:** TA1FM-7; nRF52840 MiniMesh, SX1262; geçici seri komutlu test sketch'leri.
- **LoRa:** 433.775 MHz, SF12, BW125, CR 4/5 (test düzeneği).
- **APRS-IS:** Gerçek aprsc sunucuları (filtre ve temel aktarım testlerinde); özel binary veride **yalnızca yerel TCP sunucusu**.
- **Yerel binary alıcısı:** Windows bilgisayar `192.168.1.117:14581`; iGate `192.168.1.209`. Bunlar yalnızca test sırasındaki özel ağ IP adresleridir, kalıcı kurulum bilgisi değildir.
- **Kanıt:** Arduino seri monitörü, iGate seri logları ve yerel TCP test programının byte-for-byte karşılaştırma çıktıları. Ham logların tamamı bu dosyaya eklenmedi; aşağıda kritik satırlar aktarıldı.

## Sonuç özeti

| Test | Gözlem | Sonuç |
| --- | --- | --- |
| Normal RF → APRS-IS | RX alındı, `TCP gonderildi: 61 byte (binary-safe)` | PASS (TCP yazımı) |
| Yinelenen normal RF alımı | Aynı uygun çerçeve iki kez alındı ve TCP'ye yazıldı | PASS (yerel iGate duplicate filtresi yok) |
| RF yolunda `NOGATE` | RX; `Gecersiz veya desteklenmeyen paket`; TCP gönderimi yok | PASS |
| RF yolunda `RFONLY` | RX; reddedildi; TCP gönderimi yok | PASS |
| Third-party `}` | RX; reddedildi; TCP gönderimi yok | PASS (koruyucu ret) |
| `TCPIP*` içeren RF yolu | RX; reddedildi; TCP gönderimi yok | PASS |
| Genel APRS sorgusu `?APRS?` | RX; reddedildi; TCP gönderimi yok | PASS |
| Manuel APRS-IS kapatma/açma | Kapalıyken alınan paket bağlantı sonrası yeniden gönderilmedi; yeni paket iletildi | PASS (manuel senaryo) |
| DIGI `WIDE1-1` | `WIDE1-1` → `TA1FM-10*`; RF TX OK | PASS |
| DIGI duplicate | Aynı paket yaklaşık 24 sn arayla; ikinci paket `Duplicate (30 sn)` | PASS |
| DIGI yasaklı/uygunsuz paketler | NOGATE, RFONLY, third-party, TCPIP*, genel sorgu alındı ve tekrar edilmedi | PASS (bu örnekler) |
| DIGI binary koruması | Binary paket alındı, arayüzde `DIGI BINARY ATLANDI` görüldü; RF tekrar yok | PASS |
| RF → iGate → yerel TCP binary bütünlüğü | 51 baytlık TCP test çerçevesi tam eşleşti; `00 80 FF` dahil | PASS |

### Seçilmiş log kanıtları

**Normal iGate aktarımı**
```text
[RX] 51 byte RSSI -54.0 SNR 4.5
[APRS] TA1FM-7>APZTFM,WIDE1-1:>TA1FM FILTER TEST NORMAL
[APRS-IS] TCP gonderildi: 61 byte (binary-safe)
```

**Koruyucu filtre örnekleri**
```text
[RX] 50 byte RSSI -52.0 SNR 6.8
[APRS] Gecersiz veya desteklenmeyen paket; HEX:
# NOGATE packet content observed in HEX

[RX] 66 byte RSSI -52.0 SNR 4.5
[APRS] Gecersiz veya desteklenmeyen paket; HEX:
# Third-party packet starting with } in APRS info
```
Bu ret mesajı nedenin ayrıntısını tek başına söylemez; içerikteki ilgili baytlar incelenerek hangi test paketinin alındığı belirlendi.

**Manuel APRS-IS bağlantısı**
```text
15:50:28 [OLAY] APRS-IS manuel KAPATILDI - Wi-Fi devam
15:51:06 [RX] ... TA1FM-7 normal test paketi
15:51:21 [APRS-IS] GIRIS DOGRULANDI
15:51:54 [RX] ... TA1FM-7 normal test paketi
15:51:54 [APRS-IS] TCP gonderildi: 61 byte (binary-safe)
```
Bağlantı tekrar açıldığında iGate'in kendi konum ve durum beacon'ları gönderildi; kapalı bağlantı sırasında alınan RF test paketinin geç gönderimi gözlenmedi.

**DIGI path işlemesi**
```text
[DIGI] Aday: TA1FM-7>APZTFM,TA1FM-10*:>TA1FM FILTER TEST NORMAL
[TX KONTROL] DIGI: istenen=20 dBm, POWER=0, OCP=0, limit=140.0 mA
[DIGI] RF TX OK: TA1FM-7>APZTFM,TA1FM-10*:>TA1FM FILTER TEST NORMAL
```
`POWER=0` ve `OCP=0` RadioLib dönüş kodlarıdır; gerçek yayılan güç ölçümü değildir.

**DIGI duplicate**
```text
16:00:43 [RX] ... TA1FM FILTER TEST NORMAL
16:00:47 [DIGI] RF TX OK: ...
16:01:07 [RX] ... TA1FM FILTER TEST NORMAL
16:01:07 [DIGI] Duplicate (30 sn)
```

**Binary DIGI koruması**
```text
[TEST] BINARY INFO HEX: 41 00 42 80 FF 43
[TEST] TX RESULT=0
[RX] 41 byte RSSI -43.0 SNR 6.2
[APRS] TA1FM-7>APZTFM,WIDE1-1:>BINTEST A.B..C
```
iGate web panelindeki kayıt `DIGI BINARY ATLANDI` olarak gözlendi. Serideki noktalar yalnızca görüntüleme amaçlıdır.

**Yerel TCP binary bütünlüğü**
```text
[LOCAL] Connection from 192.168.1.209
[LOCAL] Login received; local simulated verification sent
[PASS] EXACT BINARY FRAME MATCH (including NUL, 0x80, 0xFF)
[PASS] TCP frame 51 bytes; info hex: 3E 42 49 4E 54 45 53 54 20 41 00 42 80 FF 43
```
Bu sonuç RF'den gelen test bilgisinin **yerel test alıcısına gönderilen TCP baytlarına** kadar aynı kaldığını kanıtlar. Özel binary veri, gerçek APRS-IS ağına test amacıyla gönderilmedi. Yerel doğrulama cevabı gerçek aprsc sunucusundan gelmedi.

## Kapsam ve açık kalan işler

1. **Third-party APRS:** Şu an `}` ile başlayan tüm third-party bilgisi koruyucu biçimde reddediliyor. İnternet kökenli olmayan, kurallara uygun third-party paketlerin ayrıştırılması ve gerekli yönlendirme mantığı henüz uygulanmadı/test edilmedi.
2. **DIGI binary:** Mevcut yol `String` görüntüleme verisi kullandığından binary bilgi içeren çerçeveleri tekrar etmez. Bozmadan RF retransmisyon özelliği henüz uygulanmadı; mevcut ret davranışı test edildi.
3. **Diğer yollar/standart durumları:** `TCPXX`, q-construct varyantları, WIDE2-1/WIDE2-2, daha önce kullanılmış digipeater yolları, kendi çağrı işareti, bozuk/azami boy paketler vb. bu saha oturumunda ayrı ayrı test edilmedi.
4. **Ağ dayanıklılığı:** Manuel APRS-IS aç/kapa testi başarılıdır; rastgele Wi-Fi kaybı, TCP yarım-yazım, DNS ve uzun süreli hata senaryoları ayrı stres testleri gerektirir.
5. **RF koşulları:** CAD, saatlik TX sınırı, beacon/DIGI çakışmaları ve gerçek RF güç seviyesi ayrı test gerektirir.
6. **Protokol doğruluğu:** TCP `write` başarılı olması APRS-IS ağındaki nihai kabulü veya aprs.fi görünürlüğünü otomatik olarak ispatlamaz.
7. **TOCALL:** `APZTFM` deneysel kimlik olarak korunmuştur; resmî tahsis yapılmadan değiştirilmez.

## Sürüm kararı

**V1.5.9 BETA — seçilmiş saha ve yerel laboratuvar testleri başarılı.**  
**V1.5.8 STABLE — mevcut kararlı sürüm olarak korunuyor.**

APRS iGate protokolüne tam uyumluluk iddia edilmez. Kalan konular kapatılmadan V1.5.9 BETA otomatik olarak STABLE ilan edilmez.

## Kaynaklar

- [V1.5.9 BETA uyumluluk notları](V1_5_9_IGATE_UYUMLULUK_TEST_TR.md)
- [V1.5.9 BETA kodu](TA1FM_LoRa_APRS_iGate_V1_5_9_BETA.ino)
- [aprsc IGATE-HINTS](https://github.com/hessu/aprsc/blob/main/doc/IGATE-HINTS.md)
- [APRS-IS IGating](https://aprs-is.net/IGating.aspx)

73 de TA1FM.
