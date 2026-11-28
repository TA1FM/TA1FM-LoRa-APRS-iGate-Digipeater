#pragma once
// Select ONE board. Arduino IDE Tools > Board must match this target.
#define TA1FM_BOARD_ESP32_DEVKIT_30
// #define TA1FM_BOARD_HELTEC_WSL_V3

#if defined(TA1FM_BOARD_ESP32_DEVKIT_30) && defined(TA1FM_BOARD_HELTEC_WSL_V3)
  #error "Select only one TA1FM board."
#elif defined(TA1FM_BOARD_ESP32_DEVKIT_30)
  #if !defined(CONFIG_IDF_TARGET_ESP32)
    #error "Select ESP32 Dev Module in Arduino IDE."
  #endif
  #include "variants/boards/esp32_devkit_30.h"
#elif defined(TA1FM_BOARD_HELTEC_WSL_V3)
  #if !defined(CONFIG_IDF_TARGET_ESP32S3)
    #error "Select ESP32S3 Dev Module in Arduino IDE."
  #endif
  #include "variants/boards/heltec_wsl_v3.h"
#else
  #error "Select exactly one board."
#endif

// Independently select the attached radio module. No chip attached? NONE.
// ESP32 DevKit test defaults to NONE; Heltec defaults to its integrated SX1262.
#if defined(TA1FM_BOARD_ESP32_DEVKIT_30)
  #define TA1FM_RADIO_NONE
  // #define TA1FM_RADIO_GENERIC_SX1262
  // #define TA1FM_RADIO_GENERIC_SX1268
  // #define TA1FM_RADIO_EBYTE_E22_400M30S
#elif defined(TA1FM_BOARD_HELTEC_WSL_V3)
  #define TA1FM_RADIO_HELTEC_SX1262
#endif

#if (defined(TA1FM_RADIO_NONE)+defined(TA1FM_RADIO_HELTEC_SX1262)+defined(TA1FM_RADIO_GENERIC_SX1262)+defined(TA1FM_RADIO_GENERIC_SX1268)+defined(TA1FM_RADIO_EBYTE_E22_400M30S))!=1
  #error "Select exactly one TA1FM_RADIO profile."
#endif

#if defined(TA1FM_RADIO_NONE)
  #include "variants/radios/none.h"
#elif defined(TA1FM_RADIO_HELTEC_SX1262)
  #include "variants/radios/heltec_sx1262.h"
#elif defined(TA1FM_RADIO_GENERIC_SX1262)
  #include "variants/radios/generic_sx1262.h"
#elif defined(TA1FM_RADIO_GENERIC_SX1268)
  #include "variants/radios/generic_sx1268.h"
#elif defined(TA1FM_RADIO_EBYTE_E22_400M30S)
  #include "variants/radios/ebyte_e22_400m30s.h"
#endif
