#pragma once

#if defined(OLED_SCREEN_I2C)
// DISPLAY SETTINGS
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ROTATION U8G2_R2
#endif

// SPI OLED
// #define OLED_CLK 13
// #define OLED_MOSI 11
// #define OLED_CS 3
// #define OLED_DC 2
// #define OLED_RST 8

#if defined(ESP8266_KIT_A)
#define PIN_BTN_L 12
#define PIN_BTN_M 13
#define PIN_BTN_R 15
#define PIN_BUZZER 0
#define BUZZER_ACTIVE_LOW
#define FRAME_DELAY_MS 50
#define PRESSED HIGH

#elif defined(ESP8266_KIT_B)
#define PIN_BTN_L 12
#define PIN_BTN_M 13
#define PIN_BTN_R 15
#define PIN_BUZZER 0
#define BUZZER_ACTIVE_LOW
#define FRAME_DELAY_MS 50
#define PRESSED HIGH

#elif defined(ESP32)
#define PIN_BTN_L 255
#define PIN_BTN_M 255
#define PIN_BTN_R 255
#define PIN_BUZZER 255
#define FRAME_DELAY_MS 50
#define PRESSED HIGH

#elif defined(ATMEGA328P)
#define PIN_BTN_L 2
#define PIN_BTN_M 3
#define PIN_BTN_R 4
#define PIN_BUZZER 9
#define FRAME_DELAY_MS 5
#define PRESSED LOW

#elif defined(ATTINY1616)
#define PIN_BTN_L 2
#define PIN_BTN_M 3
#define PIN_BTN_R 4
#define PIN_BUZZER 9
#define FRAME_DELAY_MS 5
#define PRESSED LOW

#endif