# TA1FM LoRa APRS iGate & Digipeater — V1.5.9 BETA / APRS-IS Guvenlik Incelemesi

> **BETA / TEST EDILMEDI:** V1.5.8 calisan surum korunuyor. Bu kaynak kod Arduino IDE ile henuz derlenmedi, RF/APRS-IS cihaz testinden gecmedi. Test ve kod incelemesi tamamlanmadan canli APRS-IS aginda kullanmayin veya resmi TOCALL uyumlulugu iddia etmeyin.

## Kaynaklar

- [Hessu / aprsc IGATE-HINTS](https://github.com/hessu/aprsc/blob/main/doc/IGATE-HINTS.md)
- [APRS-IS IGate Design](https://aprs-is.net/IGating.aspx)
- [APRS-IS IGate Details](https://aprs-is.net/IGateDetails.aspx)
- [V1.5.8 resmi calisan surum](https://github.com/TA1FM/TA1FM-LoRa-APRS-iGate-Digipeater/releases/tag/v1.5.8)

## V1.5.9 BETA'da yapilan kod degisiklikleri

1. **RX iGate tekrar filtresi kaldirildi.** APRS-IS dogrulanmis ve bagliyken her uygun RF alimi bagimsiz olarak APRS-IS'e siralanir. 30 sn tekrar filtresi sadece RF DIGI modunda kullanilir.
2. **Binary-safe APRS-IS kuyrugu eklendi.** APRS bilgi alani, paket boyu ile beraber `uint8_t[]` icinde tasinir. `0x00` ve yazdirilamayan baytlar APRS-IS'e yazilirken kesilip donusturulmez; UI/seri gorunumu ayri olarak kisitlanir.
3. **Aktarim filtreleri:** `TCPIP`, `TCPXX`, `NOGATE`, `RFONLY`, APRS-IS q path'leri ve genel sorgular ayiklanir. RF'den gelen internet kaynakli ucuncu taraf paketleri geri gonderilmez.
4. **Kuyruk temizligi:** APRS-IS baglantisi dustugunde bekleyen RF paketleri temizlenir; tekrar baglandiginda eski konum paketleri gecikmeli olarak yayinlanmaz.
5. **Baglanti modeli:** Tek `WiFiClient` APRS-IS TCP baglantisi ve yeniden baglanista hostname cozumlemesi korunur; APRS-IS'ten RF'e TX eklenmedi.
6. **Surum:** Yeni test dosyasi V1.5.9 BETA olarak etiketlenir. V1.5.8 `.ino`, `.bin`, `merged.bin` ve Release degistirilmedi.

## Henuz yapilmayan ve dogrulanmasi gerekenler

- **Ucuncu taraf paketleri:** Internet-kaynakli geri-donus riskini azaltmak icin `}` ile baslayan tum third-party paketleri su an fail-closed reddediyoruz. Standart geregi internet kaynakli *olmayan* third-party APRS paketlerinin dogru sekilde acilip aktarilmasi ayrica uygulanmalidir. Bu nedenle **IGate spec ile tam uyum henuz iddia edilmiyor**.
- **RF tekrarlarinda binary bilgi:** DIGI yolunda hala gorsellestirilmis `String` paket temsili kullaniliyor; ikili veri iceren paketin RF'de bozulmadan tekrar aktarimi ayrica gozden gecirilmeli. Bu tamamlanmadan DIGI TX testini aktif etmeyin.
- **Paket boyu, LoRa CRC ve TNC2 uygulamasi:** Gercek LoRa sinirlari, uygun q yapisi, RF/IS trafik, gecerli/g gecersiz AX.25 verileri gercek donanimda dogrulanmali.
- **DNS sunucusu:** Varsayilan `euro.aprs2.net` kayitli ayarlari bozmamak icin korunmustur. Kullanici ayarlarinda `rotate.aprs2.net` gibi round-robin sunucu adinin secilmesi ve TCP baglantisinda her seferinde yeniden DNS cozumu kontrol edilmelidir.
- **TOCALL:** Mevcut `APZTFM` deneysel kimliktir. Resmi tahsis oncesinde TOCALL degistirilmemelidir.

## Derleme ve test sirasi

1. **V1.5.8 yedegi:** Mevcut `.bin`, `.ino`, ayarlar ve APRS-IS bilgisini guvenli saklayin.
2. Bu BETA `.ino` dosyasini **Arduino IDE'de derleyin**. Derleme sorunu olursa surum ve tam hata satirlarini kaydedin. Kod cihazda test edilmeden mevcut Release'in yerini almasin.
3. Ilk testte **RF TX / Auto DIGI kapali** ve mumkunse APRS-IS'e gercek paket gondermeden seri RX izlemesi yapin.
4. Kontrollu test paketi ile **aynı paket 2 kez** RF'den duyuruldugunda iGate'in ikisini de uygun sekilde APRS-IS kuyruguna aldigini kontrol edin.
5. `NOGATE`, `RFONLY`, `TCPIP`, `TCPXX`, `qAO`, `?` genel sorgular ve third-party paketlerde reddetme davranisini dogrulayin.
6. Bilgi bolumunde NUL `0x00`, yazdirilamayan baytlar ve sondaki bosluklar bulunan test verilerini, kontrollu yerel/test TCP alicisi ile **byte-for-byte** karsilastirin. Canli APRS-IS'e anlamsiz/binary test verisi gondermeyin.
7. Baglanti kopma/yeniden baglanma durumunda eskimis kuyruk paketlerinin sonradan gonderilmedigini dogrulayin.
8. Internet/APRS-IS ve RF DIGI gecislerini, TX limiti, CAD, NOGATE/RFONLY ve tekrar filtresi kontrollerini ayri test edin.

**Sonraki asama:** Third-party paket ayrisma ve RF DIGI binary-safe yeniden yayinlama tamamlanip testler onaylaninca yeni kararlı surum ve TOCALL basvurusundaki teknik uyumluluk aciklamasi hazırlanabilir.

73 de TA1FM
