// Space Invaders game based on the work of
// http://www.xtronical.com/projects/space-invaders/
// and Ricardo Moreno (check out youtube video)

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>

#include "hardware.h"
// U8G2_SSD1325_NHD_128X64_F_4W_HW_SPI display(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);  // 2.7 inch White/Black

#if defined(OLED_SCREEN_I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(SCREEN_ROTATION);
#endif

#include "game.h"
#include "spaceinvaders.h"

Game* game = new SpaceInvaders(&display);

void setup()
{
  Serial.begin(SERIAL_BAUD);
  Serial.print("Started\n");

  pinMode(PIN_BTN_L, INPUT);
  pinMode(PIN_BTN_M, INPUT);
  pinMode(PIN_BTN_R, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  #ifdef BUZZER_ACTIVE_LOW
  digitalWrite(PIN_BUZZER, HIGH);
  #endif

  display.begin();
  display.clear();
  display.setBitmapMode(1);

  game->init();
}

uint32_t frame = 0;

void loop()
{
  game->loop();
  
  delay(FRAME_DELAY_MS);
  frame++;
}