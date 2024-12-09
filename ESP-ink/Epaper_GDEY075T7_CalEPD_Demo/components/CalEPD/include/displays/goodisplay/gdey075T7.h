/*
 * This file is based on source code originally from martinberlin/CalEPD GitHub repository,
 * available at https://github.com/martinberlin/CalEPD.
 *
 * Modifications have been made to the original code by Jakub Franek (https://github.com/JakubFranek),
 * as permitted under the Apache License, Version 2.0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "epd.h"
#include "Adafruit_GFX.h"
#include "epdspi.h"
#include "gdew_4grays.h"
#include "esp_timer.h"

#define GDEY075T7_WIDTH 800
#define GDEY075T7_HEIGHT 480

#define GDEY075T7_BUFFER_SIZE (uint32_t(GDEY075T7_WIDTH) * uint32_t(GDEY075T7_HEIGHT) / 8)

typedef enum GDEY075T7_UPDATE_MODE
{
  GDEY075T7_FULL_UPDATE = 0,
  GDEY075T7_PARTIAL_UPDATE = 1,
  GDEY075T7_FAST_UPDATE = 2
} GDEY075T7_UPDATE_MODE;

typedef struct
{
  uint8_t cmd;
  uint8_t data[42];
  uint8_t databytes;
} epd_init_42;

class Gdey075T7 : public Epd
{
public:
  Gdey075T7(EpdSpi &epd_spi);
  uint8_t colors_supported = 1;

  void drawPixel(int16_t x, int16_t y, uint16_t color); // Override GFX own drawPixel method

  void initialize();

  void initPartialUpdate();
  void updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation);
  void fillScreen(uint16_t color);
  void update();

  void set_update_mode(GDEY075T7_UPDATE_MODE mode);

private:
  EpdSpi &epd_spi;
  GDEY075T7_UPDATE_MODE _update_mode = GDEY075T7_FULL_UPDATE;

  uint8_t _buffer[GDEY075T7_BUFFER_SIZE];

  bool _using_partial_mode = false;
  bool _initial = true;
  bool _is_power_on = false;
  bool _is_in_deep_sleep = true;
  bool _useFastPartialUpdateFromOTP = true;

  uint16_t _setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye);
  void _wakeUp();
  void _sleep();
  void _wait_while_busy(const char *message);
  void _rotate(uint16_t &x, uint16_t &y, uint16_t &w, uint16_t &h);
  void _InitDisplay();
  void _Init_Full();
  void _Init_Part();
  void _Update_Full();
  void _refresh();
  void _PowerOn();
  void _PowerOff();
  void _enter_deep_sleep();

  static const epd_init_42 lut_20_LUTC_partial;
  static const epd_init_42 lut_21_LUTWW_partial;
  static const epd_init_42 lut_22_LUTKW_partial;
  static const epd_init_42 lut_23_LUTWK_partial;
  static const epd_init_42 lut_24_LUTKK_partial;
  static const epd_init_42 lut_25_LUTBD_partial;
};