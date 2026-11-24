#pragma once
// ONE BOARD SELECTION ONLY. All hardware behavior derives from this.
// EDIT ONLY THIS SECTION before compiling:
#define TA1FM_BOARD_LILYGO_TBEAM_V12_SX1278
// #define TA1FM_BOARD_HELTEC_WSL_V3
// #define TA1FM_BOARD_ESP32_DEVKIT_30

#if (defined(TA1FM_BOARD_LILYGO_TBEAM_V12_SX1278)+defined(TA1FM_BOARD_HELTEC_WSL_V3)+defined(TA1FM_BOARD_ESP32_DEVKIT_30)) != 1
 #error "Select exactly one board profile in board_config.h"
#endif
#if defined(TA1FM_BOARD_LILYGO_TBEAM_V12_SX1278)
 #if !defined(CONFIG_IDF_TARGET_ESP32)
  #error "T-Beam requires classic ESP32 Dev Module"
 #endif
 #include "variants/boards/lilygo_tbeam_v12_sx1278.h"
 #include "variants/radios/tbeam_sx1278.h"
#elif defined(TA1FM_BOARD_HELTEC_WSL_V3)
 #if !defined(CONFIG_IDF_TARGET_ESP32S3)
  #error "Heltec Wireless Stick Lite V3 requires ESP32S3 Dev Module"
 #endif
 #include "variants/boards/heltec_wsl_v3.h"
 #include "variants/radios/heltec_sx1262.h"
#elif defined(TA1FM_BOARD_ESP32_DEVKIT_30)
 #if !defined(CONFIG_IDF_TARGET_ESP32)
  #error "ESP32 DevKit requires ESP32 Dev Module"
 #endif
 #include "variants/boards/esp32_devkit_30.h"
 #include "variants/radios/none.h"
#endif
