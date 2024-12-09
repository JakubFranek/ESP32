/*
 * This file is based on source code originally from martinberlin/CalEPD GitHub repository,
 * available at https://github.com/martinberlin/CalEPD.
 *
 * Modifications have been made to the original code by Jakub Franek (https://github.com/JakubFranek),
 * as permitted under the Apache License, Version 2.0.
 */

#include "displays/goodisplay/gdey075T7.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "freertos/task.h"
#include "esp_log.h"

#define UC8179_SPI_FREQUENCY_MHZ 20
#define GDEY075T7_BUSY_TIMEOUT_US 10000000 // 10 s

#define UC8179_CMD_PANEL_SETTING 0x00
#define UC8179_CMD_POWER_SETTING 0x01
#define UC8179_CMD_POWER_OFF 0x02
#define UC8179_CMD_POWER_OFF_SEQUENCE 0x03
#define UC8179_CMD_POWER_ON 0x04
#define UC8179_CMD_POWER_ON_MEASURE 0x05
#define UC8179_CMD_BOOSTER_SOFT_START 0x06
#define UC8179_CMD_DEEP_SLEEP 0x07
#define UC8179_CMD_DATA_START_TRANSMISSION_1 0x10
#define UC8179_CMD_DATA_STOP 0x11
#define UC8179_CMD_DISPLAY_REFRESH 0x12
#define UC8179_CMD_DATA_START_TRANSMISSION_2 0x13
#define UC8179_CMD_DUAL_SPI 0x15
#define UC8179_CMD_AUTO_SEQUENCE 0x17
#define UC8179_CMD_LUTC 0x20
#define UC8179_CMD_LUTWW 0x21
#define UC8179_CMD_LUTKW 0x22
#define UC8179_CMD_LUTWK 0x23
#define UC8179_CMD_LUTKK 0x24
#define UC8179_CMD_LUT_BORDER 0x25
#define UC8179_CMD_LUTOPT 0x2A
#define UC8179_CMD_KWOP 0x2B
#define UC8179_CMD_PLL_CONTROL 0x30
#define UC8179_CMD_TEMPERATURE_SENSOR_CALIBRATRION 0x40
#define UC8179_CMD_TEMPERATURE_SENSOR_SELECTION 0x41
#define UC8179_CMD_TEMPERATURE_SENSOR_WRITE 0x42
#define UC8179_CMD_TEMPERATURE_SENSOR_READ 0x43
#define UC8179_CMD_PANEL_BREAK_CHECK 0x44
#define UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING 0x50
#define UC8179_CMD_LOWER_POWER_DETECTION 0x51
#define UC8179_CMD_END_VOLTAGE_SETTING 0x52
#define UC8179_CMD_TCON_SETTING 0x60
#define UC8179_CMD_RESOLUTION_SETTING 0x61
#define UC8179_CMD_GATE_SOURCE_START_SETTING 0x65
#define UC8179_CMD_REVISION 0x70
#define UC8179_CMD_GET_STATUS 0x71
#define UC8179_CMD_AUTO_MEASUREMENT_VCOM 0x80
#define UC8179_CMD_READ_VCOM 0x81
#define UC8179_CMD_VCOM_DC_SETTING 0x82
#define UC8179_CMD_PARTIAL_WINDOW 0x90
#define UC8179_CMD_PARTIAL_IN 0x91
#define UC8179_CMD_PARTIAL_OUT 0x92
#define UC8179_CMD_PROGRAM_MODE 0xA0
#define UC8179_CMD_ACTIVE_PROGRAMMING 0xA1
#define UC8179_CMD_READ_OTP 0xA2
#define UC8179_CMD_CASCADE_SETTING 0xE0
#define UC8179_CMD_POWER_SAVING 0xE3
#define UC8179_CMD_LVD_VOLTAGE_SELECT 0xE4
#define UC8179_CMD_FORCE_TEMPERATURE 0xE5
#define UC8179_CMD_TEMPERATURE_BOUNDARY_PHASE_C2 0xE7

/*
 The EPD needs a bunch of command/data values to be initialized. They are send using the epd_spi class
 Manufacturer sample: https://github.com/waveshare/e-Paper/blob/master/Arduino/epd7in5_V2/epd7in5_V2.cpp
*/
#define T1 0x19 // charge balance pre-phase
#define T2 0x01 // optional extension
#define T3 0x00 // color change phase (b/w)
#define T4 0x00 // optional extension for one color

// Partial Update Delay, may have an influence on degradation
#define GDEY075T7_PU_DELAY 100

// Partial display Waveform
DRAM_ATTR const epd_init_42 Gdey075T7::lut_20_LUTC_partial = {
    UC8179_CMD_LUTC, {0x00, T1, T2, T3, T4, 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 42};

DRAM_ATTR const epd_init_42 Gdey075T7::lut_21_LUTWW_partial = {
    UC8179_CMD_LUTWW, {0x00, T1, T2, T3, T4, 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 42};

DRAM_ATTR const epd_init_42 Gdey075T7::lut_22_LUTKW_partial = {
    UC8179_CMD_LUTKW, {0x80, T1, T2, T3, T4, 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 42};

DRAM_ATTR const epd_init_42 Gdey075T7::lut_23_LUTWK_partial = {
    UC8179_CMD_LUTWK, {0x40, T1, T2, T3, T4, 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 42};

DRAM_ATTR const epd_init_42 Gdey075T7::lut_24_LUTKK_partial = {
    UC8179_CMD_LUTKK, {0x00, T1, T2, T3, T4, 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 42};

DRAM_ATTR const epd_init_42 Gdey075T7::lut_25_LUTBD_partial = {
    UC8179_CMD_LUT_BORDER, {0x00, T1, T2, T3, T4, 1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 42};

Gdey075T7::Gdey075T7(EpdSpi &epd_spi) : Adafruit_GFX(GDEY075T7_WIDTH, GDEY075T7_HEIGHT),
                                        Epd(GDEY075T7_WIDTH, GDEY075T7_HEIGHT), epd_spi(epd_spi) {};

void Gdey075T7::initPartialUpdate()
{
  epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
  epd_spi.send_data(0x3F); // LUT selection: REG; B&W mode; Gate scan direction: up; Source shift direction: right; Booster: on; Soft reset: no effect

  epd_spi.send_command(UC8179_CMD_VCOM_DC_SETTING);
  epd_spi.send_data(0x26); // VCOM -2.00 V

  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0x39); // Border output: Hi-Z;Border LUT selection: LUTBD; Copy new data to old data after display refresh: enabled
  epd_spi.send_data(0x07); // VCOM and data interval: 0d10

  // LUT Tables for partial update, sent in 42 byte chunks, total 210 bytes
  epd_spi.send_command(lut_20_LUTC_partial.cmd);
  epd_spi.send_data(lut_20_LUTC_partial.data, lut_20_LUTC_partial.databytes);

  epd_spi.send_command(lut_21_LUTWW_partial.cmd);
  epd_spi.send_data(lut_21_LUTWW_partial.data, lut_21_LUTWW_partial.databytes);

  epd_spi.send_command(lut_22_LUTKW_partial.cmd);
  epd_spi.send_data(lut_22_LUTKW_partial.data, lut_22_LUTKW_partial.databytes);

  epd_spi.send_command(lut_23_LUTWK_partial.cmd);
  epd_spi.send_data(lut_23_LUTWK_partial.data, lut_23_LUTWK_partial.databytes);

  epd_spi.send_command(lut_24_LUTKK_partial.cmd);
  epd_spi.send_data(lut_24_LUTKK_partial.data, lut_24_LUTKK_partial.databytes);

  epd_spi.send_command(lut_25_LUTBD_partial.cmd);
  epd_spi.send_data(lut_25_LUTBD_partial.data, lut_25_LUTBD_partial.databytes);
}

void Gdey075T7::initialize()
{
  epd_spi.initialize(UC8179_SPI_FREQUENCY_MHZ);
  fillScreen(EPD_WHITE);
  _wakeUp();
}

void Gdey075T7::fillScreen(uint16_t color)
{
  uint8_t data = (color == EPD_BLACK) ? EPD_BLACK : EPD_WHITE;
  for (uint16_t x = 0; x < sizeof(_buffer); x++)
  {
    _buffer[x] = data;
  }
}

void Gdey075T7::_wakeUp()
{
  epd_spi.hardware_reset(10);

  epd_spi.send_command(UC8179_CMD_POWER_SETTING);
  epd_spi.send_data(0x07); // (default) Source LV power selection: internal; Source power selection: internal; Gate power selection: internal
  epd_spi.send_data(0x07); // VGH / VGL selection: VGH = 20 V, VGL = -20 V (default)
  epd_spi.send_data(0x3f); // VDH selection: 15 V
  epd_spi.send_data(0x3f); // VDL selection: -15 V
  epd_spi.send_data(0x09); // VDHR selection: 4.2 V

  epd_spi.send_command(UC8179_CMD_BOOSTER_SOFT_START);
  epd_spi.send_data(0x17); // (default) Soft start phase A: 10 ms; Strength phase A: 3; Min. off time of GDR in phase A: 6.58 us
  epd_spi.send_data(0x17); // (default) Soft start phase B: 10 ms; Strength phase B: 3; Min. off time of GDR in phase B: 6.58 us
  epd_spi.send_data(0x28); // Strength phase C1: 6; Min. off time of GDR in phase C1: 0.27 us
  epd_spi.send_data(0x17); // (default) Booster phase C2: disabled; Strength phase C2: 3; Min. off time of GDR in phase C2: 6.58 us

  epd_spi.send_command(UC8179_CMD_POWER_ON); // POWER ON

  _waitBusy("Power On command (_wakeUp)");

  epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
  epd_spi.send_data(0x1F); // LUT selection: OTP; B&W mode; Gate scan direction: up; Source shift direction: right; Booster: on; Soft reset: no effect

  epd_spi.send_command(UC8179_CMD_RESOLUTION_SETTING);
  epd_spi.send_data(GDEY075T7_WIDTH / 256);
  epd_spi.send_data(GDEY075T7_WIDTH % 256);
  epd_spi.send_data(GDEY075T7_HEIGHT / 256);
  epd_spi.send_data(GDEY075T7_HEIGHT % 256);

  epd_spi.send_command(UC8179_CMD_DUAL_SPI);
  epd_spi.send_data(0x00); // (default) Dual SPI mode: disabled

  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0x10); // Border output: Hi-Z; Border LUT selection: LUTW; Copy new data to old data after display refresh: disabled
  // epd_spi.send_data(0x29) // TODO: find out which value is better (CalEPD vs. GxEPD2)
  epd_spi.send_data(0x07); // VCOM and data interval: 0d10

  epd_spi.send_command(UC8179_CMD_TCON_SETTING);
  epd_spi.send_data(0x22); // Source to Gate non-overlap: 0d12; Gate to Source non-overlap: 0d12

  epd_spi.send_command(UC8179_CMD_POWER_SAVING);
  epd_spi.send_data(0x22); // VCOM Power Saving: 0d2 line period; Source power saving: 2*660 ns
}

void Gdey075T7::update()
{
  _using_partial_mode = false;
  _wakeUp();

  epd_spi.send_command(UC8179_CMD_DATA_START_TRANSMISSION_2);

  uint16_t i = 0;
  uint8_t xLineBytes = GDEY075T7_WIDTH / 8;
  uint8_t x1buf[xLineBytes];
  for (uint16_t y = 1; y <= GDEY075T7_HEIGHT; y++)
  {
    for (uint16_t x = 1; x <= xLineBytes; x++)
    {
      uint8_t data = i < sizeof(_buffer) ? ~_buffer[i] : 0x00;
      x1buf[x - 1] = data;
      if (x == xLineBytes)
      {
        epd_spi.send_data(x1buf, sizeof(x1buf));
      }
      ++i;
    }
  }

  epd_spi.send_command(UC8179_CMD_DISPLAY_REFRESH);
  _waitBusy("Display Refresh command (_update)");
  _sleep();
}

uint16_t Gdey075T7::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye)
{
  x &= 0xFFF8;            // byte boundary
  xe = (xe - 1) | 0x0007; // byte boundary - 1

  epd_spi.send_command(UC8179_CMD_PARTIAL_WINDOW);
  epd_spi.send_data(x / 256);
  epd_spi.send_data(x % 256);
  epd_spi.send_data(xe / 256);
  epd_spi.send_data(xe % 256);
  epd_spi.send_data(y / 256);
  epd_spi.send_data(y % 256);
  epd_spi.send_data(ye / 256);
  epd_spi.send_data(ye % 256);
  epd_spi.send_data(0x00);

  return (7 + xe - x) / 8; // number of bytes to transfer per line
}

void Gdey075T7::updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation)
{
  if (using_rotation)
    _rotate(x, y, w, h);
  if (x >= GDEY075T7_WIDTH)
    return;
  if (y >= GDEY075T7_HEIGHT)
    return;
  uint16_t xe = gx_uint16_min(GDEY075T7_WIDTH, x + w) - 1;
  uint16_t ye = gx_uint16_min(GDEY075T7_HEIGHT, y + h) - 1;

  uint16_t xs_bx = x / 8;
  uint16_t xe_bx = (xe + 7) / 8;
  if (!_using_partial_mode)
  {
    _wakeUp();
  }

  _using_partial_mode = true;
  initPartialUpdate();

  epd_spi.send_command(UC8179_CMD_PARTIAL_IN); // partial in
  _setPartialRamArea(x, y, xe, ye);
  epd_spi.send_command(UC8179_CMD_DATA_START_TRANSMISSION_2);

  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_bx; x1 < xe_bx; x1++)
    {
      uint16_t idx = y1 * (GDEY075T7_WIDTH / 8) + x1;
      // white is 0x00 in buffer
      uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
      // white is 0xFF on device
      epd_spi.send_data(data);

      if (idx % 8 == 0)
      {
        vTaskDelay(pdMS_TO_TICKS(1));
      }
    }
  }
  epd_spi.send_command(UC8179_CMD_DISPLAY_REFRESH);
  _waitBusy("Display Refresh command (updateWindow)");
  epd_spi.send_command(UC8179_CMD_PARTIAL_OUT);

  vTaskDelay(GDEY075T7_PU_DELAY / portTICK_PERIOD_MS);
}

void Gdey075T7::_waitBusy(const char *message)
{
  int64_t time_since_boot = esp_timer_get_time();

  while (1)
  {
    if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 1)
      break;

    vTaskDelay(10 / portTICK_PERIOD_MS);

    if (esp_timer_get_time() - time_since_boot > GDEY075T7_BUSY_TIMEOUT_US)
    {
      ESP_LOGW(TAG, "Busy Timeout: %s", message);
      break;
    }
  }
}

void Gdey075T7::_sleep()
{
  epd_spi.send_command(UC8179_CMD_POWER_OFF);
  _waitBusy("Power off command (_sleep)");
  epd_spi.send_command(UC8179_CMD_DEEP_SLEEP);
  epd_spi.send_data(0xA5); // Magic number to enter deep sleep as defined in UC8179 datasheet
}

void Gdey075T7::_rotate(uint16_t &x, uint16_t &y, uint16_t &w, uint16_t &h)
{
  switch (getRotation())
  {
  case 1:
    swap(x, y);
    swap(w, h);
    x = GDEY075T7_WIDTH - x - w - 1;
    break;
  case 2:
    x = GDEY075T7_WIDTH - x - w - 1;
    y = GDEY075T7_HEIGHT - y - h - 1;
    break;
  case 3:
    swap(x, y);
    swap(w, h);
    y = GDEY075T7_HEIGHT - y - h - 1;
    break;
  }
}

void Gdey075T7::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
    return;

  switch (getRotation())
  {
  case 0:
    break;
  case 1:
    swap(x, y);
    x = GDEY075T7_WIDTH - x - 1;
    break;
  case 2:
    x = GDEY075T7_WIDTH - x - 1;
    y = GDEY075T7_HEIGHT - y - 1;
    break;
  case 3:
    swap(x, y);
    y = GDEY075T7_HEIGHT - y - 1;
    break;
  }

  uint16_t i = x / 8 + y * GDEY075T7_WIDTH / 8;

  if (color)
  {
    _buffer[i] = (_buffer[i] | (1 << (7 - x % 8)));
  }
  else
  {
    _buffer[i] = (_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
  }
}
