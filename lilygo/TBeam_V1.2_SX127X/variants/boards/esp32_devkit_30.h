#pragma once
// GENERIC 30-PIN CLASSIC ESP32 DevKit (ESP32-WROOM-32) TEST PROFILE.
// Check the silk-screen pins on YOUR board before wiring.
// No LoRa module initially: radio.begin() may fail, but APRS-IS stays available.
// Later add a 3.3 V SX1262 module using the pins below and restart.
static constexpr const char* TA1FM_BOARD_NAME="ESP32 DevKit 30-pin WROOM";
static constexpr int PIN_SCK=18, PIN_MISO=19, PIN_MOSI=23, PIN_CS=5;
static constexpr int PIN_RST=27, PIN_DIO1=26, PIN_BUSY=25;
static constexpr int PIN_SDA=21, PIN_SCL=22, PIN_VEXT=-1;
static constexpr int GPS_RX=16, GPS_TX=17;
static constexpr int OLED_BUTTON_PIN=0;  // BOOT (GPIO0), active LOW; don't hold at power-on.
static constexpr uint32_t GPS_BAUD=9600;
// Generic DevKit has no verified on-board battery divider.
// Leave disabled to avoid false Vbat readings; other functionality stays intact.
static constexpr int BATTERY_PIN=-1, ADC_CTRL_PIN=-1;
static constexpr int ADC_CTRL_ON_STATE=LOW;
static constexpr float BATTERY_DIVIDER_RATIO=1.0f;

#define TA1FM_BATTERY_METHOD 0
static constexpr int TA1FM_RF_RXEN=-1,TA1FM_RF_TXEN=-1;
