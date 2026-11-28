#pragma once
// Original, field-tested DEV8.1 Heltec Wireless Stick Lite V3 pinout.
static constexpr const char* TA1FM_BOARD_NAME="Heltec WSL V3 ESP32-S3";
static constexpr int PIN_SCK=9, PIN_MISO=11, PIN_MOSI=10, PIN_CS=8;
static constexpr int PIN_RST=12, PIN_DIO1=14, PIN_BUSY=13;
static constexpr int PIN_SDA=41, PIN_SCL=42, PIN_VEXT=36;
static constexpr int GPS_RX=47, GPS_TX=48;
static constexpr int OLED_BUTTON_PIN=0;
static constexpr uint32_t GPS_BAUD=9600;
static constexpr int BATTERY_PIN=1, ADC_CTRL_PIN=37;
static constexpr int ADC_CTRL_ON_STATE=LOW;
static constexpr float BATTERY_DIVIDER_RATIO=4.9f;

#define TA1FM_BATTERY_METHOD 1
static constexpr int TA1FM_RF_RXEN=-1,TA1FM_RF_TXEN=-1;
