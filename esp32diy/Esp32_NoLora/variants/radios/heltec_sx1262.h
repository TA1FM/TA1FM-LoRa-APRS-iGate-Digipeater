#pragma once
// Radio profile. Generic/1W TX is blocked until RF switch, PA, OCP,
// power supply, module pinout and antenna are independently validated.
static constexpr const char* TA1FM_RADIO_NAME="Heltec integrated SX1262";
static constexpr bool TA1FM_RADIO_ENABLED=true;
static constexpr bool TA1FM_RADIO_HELTEC_RF_SETUP=true;
static constexpr bool TA1FM_RADIO_TX_VERIFIED=true;
static constexpr bool TA1FM_RADIO_FAMILY_SX1268=false;
static constexpr float TA1FM_RADIO_TCXO_VOLTAGE=1.8f;
