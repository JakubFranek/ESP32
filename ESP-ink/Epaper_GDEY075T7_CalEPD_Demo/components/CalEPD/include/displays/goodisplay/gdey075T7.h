/*
 * This file is based on source code originally from martinberlin/CalEPD GitHub repository,
 * available at https://github.com/martinberlin/CalEPD.
 * Modifications have been made to the original code by Jakub Franek (https://github.com/JakubFranek),
 * as permitted under the Apache License, Version 2.0.
 *
 * This file includes code adapted from GxEPD2 by Jean-Marc Zingg.
 * Original source: https://github.com/ZinggJM/GxEPD2
 * Licensed under the GNU General Public License v3.0 (GPLv3).
 * See the LICENSE file or https://www.gnu.org/licenses/gpl-3.0.en.html for details.
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
  GDEY075T7_NORMAL_UPDATE = 0,
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

  void drawPixel(int16_t x, int16_t y, uint16_t color); // Override Adafruit_GFX drawPixel method
  void fillScreen(uint16_t color);                      // Override Adafruit_GFX fillScreen method

  void initialize();
  void update();
  void set_update_mode(GDEY075T7_UPDATE_MODE mode);
  void clear_screen();

  void EPD_Dis_PartAll(const unsigned char *datas);
  void EPD_Init_Part(void);
  void EPD_WhiteScreen_White_Basemap(void);
  void EPD_Init(void);

  uint8_t _buffer[GDEY075T7_BUFFER_SIZE];

private:
  EpdSpi &epd_spi;
  GDEY075T7_UPDATE_MODE update_mode_ = GDEY075T7_NORMAL_UPDATE;

  bool is_power_on_ = false;
  bool is_in_deep_sleep_ = true;
  bool use_partial_update_from_otp_ = true;

  void set_partial_ram_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void transfer_buffer_(uint8_t command);
  void wait_while_busy_(const char *message);
  void rotate_(uint16_t &x, uint16_t &y, uint16_t &w, uint16_t &h);
  void initialize_display_common_();
  void initialize_normal_update_();
  void initialize_fast_update_();
  void initialize_partial_update_();
  void send_refresh_command_();
  void power_on_();
  void power_off_();
  void sleep_();
  void refresh_();
  void refresh_(int16_t x, int16_t y, int16_t w, int16_t h);
  void update_partial_();

  static const epd_init_42 lut_20_LUTC_partial;
  static const epd_init_42 lut_21_LUTWW_partial;
  static const epd_init_42 lut_22_LUTKW_partial;
  static const epd_init_42 lut_23_LUTWK_partial;
  static const epd_init_42 lut_24_LUTKK_partial;
  static const epd_init_42 lut_25_LUTBD_partial;
};