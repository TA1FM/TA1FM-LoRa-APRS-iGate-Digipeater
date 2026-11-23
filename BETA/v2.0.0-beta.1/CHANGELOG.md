# Changelog

## v2.0.0-beta.1 — 2026-11-23
**Pre-release source / saha testi. DEV7.2 referansi.**

- GPS UART and live GNSS status; adjustable tracker beacon modes (off/RF/APRS-IS/both)
- Speed-based SmartBeaconing and turn-trigger configuration
- Independent tracker status, mobile iGate GPS position
- APRS messages on RF/APRS-IS; log includes sent/received messages, periodic web refresh and ACK status
- Optional restricted APRS-IS -> RF message bridge, recent-heard callsign gating
- BME280 TLM with absent/disconnected/invalid data inhibition
- Multi-Wi-Fi support including phone hotspot
- New system view with profiles, security, OTA quick access, status, and separate backup tab
- Expanded JSON backup/restore for newer preferences
- Restored alignment of System page card columns (DEV7.2)
- GitHub packaging moves default secrets to an untracked local header

### Validation
- User reported successful compilation/device use for DEV7.2 and positive messaging/bridge tests.
- Long-term endurance, all feature combinations, restored backups, and builds of **this packaged GitHub version** remain unverified.

### Upgrade warning
This is source-only. Existing stable/main files and published binaries remain unchanged. Before flash/OTA, back up settings and save known-working firmware.
