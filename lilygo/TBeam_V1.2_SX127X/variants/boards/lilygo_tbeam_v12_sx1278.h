#pragma once
// Derived from user's successful SX127x RegVersion 0x12, Meshtastic boot log.
// This particular V1.2 has AXP2101 PMU and SSD1306 at 0x3C.
static constexpr const char* TA1FM_BOARD_NAME="LILYGO T-Beam V1.2 AXP2101";
static constexpr int PIN_SCK=5,PIN_MISO=19,PIN_MOSI=27,PIN_CS=18;
static constexpr int PIN_RST=23,PIN_DIO1=26,PIN_BUSY=-1; // DIO1 variable means IRQ; SX127x IRQ is DIO0/GPIO26
static constexpr int PIN_SDA=21,PIN_SCL=22,PIN_VEXT=-1;
static constexpr int GPS_RX=34,GPS_TX=12;
static constexpr int OLED_BUTTON_PIN=38; // T-Beam physical button; check actual board
static constexpr uint32_t GPS_BAUD=9600;
static constexpr int BATTERY_PIN=-1,ADC_CTRL_PIN=-1,ADC_CTRL_ON_STATE=LOW;
static constexpr float BATTERY_DIVIDER_RATIO=1.0f;
#define TA1FM_BATTERY_METHOD 2
static constexpr int TA1FM_RF_RXEN=14,TA1FM_RF_TXEN=13;
