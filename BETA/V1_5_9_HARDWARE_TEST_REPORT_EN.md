# TA1FM LoRa APRS iGate & Digipeater — V1.5.9 BETA Hardware Test Report

**Test date:** November 17, 2026  
**Status:** The specified test scenarios passed. This is **not** a claim of full APRS protocol compliance or approval to publish a STABLE release.  
**Firmware:** `BETA/TA1FM_LoRa_APRS_iGate_V1_5_9_BETA.ino`  
**Previous stable version:** V1.5.8 STABLE (unchanged).

## Test setup

- **iGate / Digipeater:** TA1FM-10, Heltec Wireless Stick Lite V3 (ESP32-S3, SX1262).
- **RF test transmitter:** TA1FM-7, nRF52840 MiniMesh with SX1262, running temporary serial-command test sketches.
- **LoRa configuration:** 433.775 MHz, SF12, BW125, CR 4/5.
- **APRS-IS:** Live aprsc servers for filtering and ordinary forwarding tests; **local TCP receiver only** for specially crafted binary test data.
- **Local binary receiver:** Windows PC at `192.168.1.117:14581`; iGate at `192.168.1.209`. These private LAN addresses were valid during testing, not permanent deployment configuration.
- **Evidence:** Arduino serial monitor and iGate logs, plus byte-for-byte output from a local TCP test program. Selected log extracts are reproduced below; this report does not contain complete raw logs.

## Results summary

| Test | Observation | Result |
| --- | --- | --- |
| Normal RF → APRS-IS | RX observed; `TCP gonderildi: 61 byte (binary-safe)` | **PASS** (TCP write) |
| Duplicate normal RF receptions | Two receptions of the same eligible frame were written to TCP | **PASS** (no local iGate duplicate suppression) |
| `NOGATE` RF path | RX observed; `Gecersiz veya desteklenmeyen paket` (invalid or unsupported packet); no TCP forwarding | **PASS** |
| `RFONLY` RF path | RX observed; rejected; no TCP forwarding | **PASS** |
| Third-party APRS information beginning with `}` | RX observed; rejected; no TCP forwarding | **PASS** (protective rejection) |
| `TCPIP*` in RF path | RX observed; rejected; no TCP forwarding | **PASS** |
| Generic APRS query `?APRS?` | RX observed; rejected; no TCP forwarding | **PASS** |
| Manual APRS-IS disable/re-enable | RF packet received while disconnected was not forwarded later; new packet was forwarded | **PASS** (manual scenario) |
| Digipeater `WIDE1-1` | `WIDE1-1` → `TA1FM-10*`; RF TX OK | **PASS** |
| Digipeater duplicate suppression | Same packet received approximately 24 seconds apart; second logged `Duplicate (30 sn)` | **PASS** |
| Digipeater disallowed/unsupported packets | NOGATE, RFONLY, third-party, TCPIP* and generic query packets were received but not repeated | **PASS** (tested cases) |
| Digipeater binary safeguard | Binary packet received; web UI showed `DIGI BINARY ATLANDI`; no RF repetition | **PASS** |
| RF → iGate → local TCP binary integrity | Full 51-byte TCP test frame matched exactly, including `00 80 FF` | **PASS** |

## Selected log evidence

### Normal iGate forwarding

```text
[RX] 51 byte RSSI -54.0 SNR 4.5
[APRS] TA1FM-7>APZTFM,WIDE1-1:>TA1FM FILTER TEST NORMAL
[APRS-IS] TCP gonderildi: 61 byte (binary-safe)
```

### Protective filter examples

```text
[RX] 50 byte RSSI -52.0 SNR 6.8
[APRS] Gecersiz veya desteklenmeyen paket; HEX:
# NOGATE packet content observed in HEX

[RX] 66 byte RSSI -52.0 SNR 4.5
[APRS] Gecersiz veya desteklenmeyen paket; HEX:
# Third-party packet starting with } in APRS info
```

The generic rejection message alone does not identify which rule was triggered; test packets were identified by inspecting the corresponding packet bytes.

### Manual APRS-IS reconnection

```text
15:50:28 [OLAY] APRS-IS manuel KAPATILDI - Wi-Fi devam
15:51:06 [RX] ... TA1FM-7 normal test packet
15:51:21 [APRS-IS] GIRIS DOGRULANDI
15:51:54 [RX] ... TA1FM-7 normal test packet
15:51:54 [APRS-IS] TCP gonderildi: 61 byte (binary-safe)
```

After reconnection, the iGate sent its own position and status beacons. The RF test packet received while the APRS-IS link was disabled was **not observed** being forwarded belatedly.

### Digipeater path processing

```text
[DIGI] Aday: TA1FM-7>APZTFM,TA1FM-10*:>TA1FM FILTER TEST NORMAL
[TX KONTROL] DIGI: istenen=20 dBm, POWER=0, OCP=0, limit=140.0 mA
[DIGI] RF TX OK: TA1FM-7>APZTFM,TA1FM-10*:>TA1FM FILTER TEST NORMAL
```

`POWER=0` and `OCP=0` are RadioLib return codes; they are **not** measurements of actual transmitted RF power.

### Digipeater duplicate suppression

```text
16:00:43 [RX] ... TA1FM FILTER TEST NORMAL
16:00:47 [DIGI] RF TX OK: ...
16:01:07 [RX] ... TA1FM FILTER TEST NORMAL
16:01:07 [DIGI] Duplicate (30 sn)
```

### Digipeater binary safeguard

```text
[TEST] BINARY INFO HEX: 41 00 42 80 FF 43
[TEST] TX RESULT=0
[RX] 41 byte RSSI -43.0 SNR 6.2
[APRS] TA1FM-7>APZTFM,WIDE1-1:>BINTEST A.B..C
```

The iGate web panel showed `DIGI BINARY ATLANDI` (binary digipeat skipped). The dots in the serial output are for display only.

### Local TCP binary-integrity test

```text
[LOCAL] Connection from 192.168.1.209
[LOCAL] Login received; local simulated verification sent
[PASS] EXACT BINARY FRAME MATCH (including NUL, 0x80, 0xFF)
[PASS] TCP frame 51 bytes; info hex: 3E 42 49 4E 54 45 53 54 20 41 00 42 80 FF 43
```

This demonstrates that the RF test information bytes were preserved through the iGate's processing and outgoing TCP connection to the **local test receiver**. Special binary test data was **not** sent to the public APRS-IS network. The simulated login verification was not issued by a real aprsc server.

## Scope limits and outstanding work

1. **Third-party APRS:** All information fields beginning with `}` are currently rejected as a protective measure. Correct parsing and forwarding decisions for legitimate third-party frames that are not Internet-originated remain to be implemented and tested.
2. **Binary RF digipeating:** The present digipeater path uses a display-oriented `String` representation and therefore does not retransmit frames containing binary information. Lossless binary RF retransmission is not implemented; only the protective rejection behavior was tested.
3. **Other paths and protocol cases:** `TCPXX`, q-construct variants, WIDE2-1/WIDE2-2, previously used digipeater paths, the station's own callsign, malformed frames, and maximum-length frames were not individually tested in this session.
4. **Network resilience:** Manual APRS-IS toggling passed. Unexpected Wi-Fi loss, partial TCP writes, DNS behavior and prolonged error scenarios require separate stress testing.
5. **RF conditions:** CAD/channel-busy behavior, hourly TX limits, beacon/digipeat contention and actual RF output power require separate tests.
6. **Protocol acceptance:** A successful TCP `write` does **not** by itself establish final acceptance by the APRS-IS network or appearance on aprs.fi.
7. **TOCALL:** `APZTFM` remains an experimental identifier and will not be changed before an official allocation.

## Release decision

**V1.5.9 BETA — the selected field and local laboratory tests passed.**  
**V1.5.8 STABLE — retained as the existing stable version.**

Full APRS iGate protocol compliance is **not claimed**. V1.5.9 BETA must not automatically be designated STABLE before outstanding issues have been addressed.

## References

- [V1.5.9 BETA compatibility notes (Turkish)](V1_5_9_IGATE_UYUMLULUK_TEST_TR.md)
- [V1.5.9 BETA firmware source](TA1FM_LoRa_APRS_iGate_V1_5_9_BETA.ino)
- [aprsc IGATE-HINTS](https://github.com/hessu/aprsc/blob/main/doc/IGATE-HINTS.md)
- [APRS-IS IGating](https://aprs-is.net/IGating.aspx)

73 de TA1FM.
