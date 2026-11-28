#pragma once
// Radio profile. Generic/1W TX is blocked until RF switch, PA, OCP,
// power supply, module pinout and antenna are independently validated.
static constexpr const char* TA1FM_RADIO_NAME="None (APRS-IS only)";
static constexpr bool TA1FM_RADIO_ENABLED=false;
static constexpr bool TA1FM_RADIO_HELTEC_RF_SETUP=false;
static constexpr bool TA1FM_RADIO_TX_VERIFIED=false;
#define TA1FM_RADIO_FAMILY_SX127X 0
#define TA1FM_RADIO_FAMILY_SX1268 0
static constexpr float TA1FM_RADIO_TCXO_VOLTAGE=0.0f;
