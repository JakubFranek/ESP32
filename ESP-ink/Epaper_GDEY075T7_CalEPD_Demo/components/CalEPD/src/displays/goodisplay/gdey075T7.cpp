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

/**
 * @brief Set up SPI bus, initialize the display and clear the screen to white.
 */
void Gdey075T7::initialize()
{
  ESP_LOGI(TAG, "initialize");
  epd_spi.initialize(UC8179_SPI_FREQUENCY_MHZ);

  /*set_update_mode(GDEY075T7_NORMAL_UPDATE);

  fillScreen(EPD_BLACK);
  transfer_buffer_(UC8179_CMD_DATA_START_TRANSMISSION_1); // Send white data to "previous" RAM
  fillScreen(EPD_WHITE);
  transfer_buffer_(UC8179_CMD_DATA_START_TRANSMISSION_2); // Send screen buffer (also all white) to "current" RAM
  refresh_();*/
}

/**
 * @brief Fills the screen with a given color.
 *
 * This function overrides `Adafruit_GFX::fillScreen()` for performance reasons.
 *
 * @param color The color to use to fill the screen. This is either EPD_WHITE or EPD_BLACK.
 */
void Gdey075T7::fillScreen(uint16_t color)
{
  ESP_LOGI(TAG, "fillScreen, color = 0x%04x", color);
  uint8_t data = (color == EPD_BLACK) ? EPD_BLACK : EPD_WHITE;
  for (uint16_t i = 0; i < sizeof(_buffer); i++)
  {
    _buffer[i] = data;
  }
}

/**
 * @brief Transfer the screen buffer to the display controller, refresh the screen and enter deep sleep.
 */
void Gdey075T7::update()
{
  ESP_LOGI(TAG, "update");
  EPD_Init_Part();
  EPD_Dis_PartAll(_buffer);
}

/**
 * @brief Transfer the screen buffer to the display controller.
 *
 * @param command The command to send to the display controller.
 */
void Gdey075T7::transfer_buffer_(uint8_t command)
{
  ESP_LOGI(TAG, "Transferring buffer, command = 0x%02x", command);
  epd_spi.send_command(UC8179_CMD_DATA_START_TRANSMISSION_2);

  uint16_t i = 0;
  uint8_t data;
  uint8_t xLineBytes = GDEY075T7_WIDTH / 8;
  uint8_t x1buf[xLineBytes];
  for (uint16_t y = 1; y <= GDEY075T7_HEIGHT; y++)
  {

    for (uint16_t x = 1; x <= xLineBytes; x++)
    {
      data = (i < sizeof(_buffer)) ? _buffer[i] : (uint8_t)EPD_WHITE;

      x1buf[x - 1] = data;

      if (x == xLineBytes)
      {
        epd_spi.send_data(x1buf, sizeof(x1buf));
      }
      i++;
    }
  }
}

/**
 * @brief Waits until the E-Ink display is no longer busy or a timeout occurs.
 *
 * This function continuously checks the busy status of the E-Ink display by reading
 * the level of the busy GPIO pin. It waits until the pin indicates that the display
 * is no longer busy. If the display remains busy beyond a specified timeout period,
 * a warning message is logged.
 *
 * @param message A message to log in case of a busy timeout.
 */
void Gdey075T7::wait_while_busy_(const char *message)
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

/**
 * @brief Applies the current rotation setting to the given coordinates.
 *
 * This internal helper function is used to rotate the coordinates of a rectangular
 * area before sending it to the E-Ink display controller. The rotation is applied
 * in-place, i.e., the input parameters are modified directly. The rotation is
 * performed relative to the top-left corner of the display.
 *
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
void Gdey075T7::rotate_(uint16_t &x, uint16_t &y, uint16_t &w, uint16_t &h)
{
  ESP_LOGI(TAG, "rotate_, rotation = %d", getRotation());
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

/**
 * @brief Draws a pixel on the E-Ink display at the specified coordinates with the given color.
 *
 * This function sets a pixel in the display buffer at the specified (x, y) coordinates.
 * It checks the current rotation setting and adjusts the coordinates accordingly.
 * If the coordinates are out of bounds, the function returns without altering the buffer.
 *
 * @param x The x-coordinate of the pixel to draw.
 * @param y The y-coordinate of the pixel to draw.
 * @param color The color of the pixel to draw. Typically `EPD_WHITE` or `EPD_BLACK`.
 */
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

/**
 * @brief Initializes common display settings for the E-Ink display.
 *
 * This function sets up essential power and resolution configurations
 * for the display. It configures the internal power settings, resolution
 * dimensions, and disables dual SPI mode.
 *
 * Power settings include source LV power, VGH/VGL, VDH, VDL, and VDHR selections.
 * Resolution settings are set using the display's width and height parameters.
 * Dual SPI mode is disabled by default.
 */
void Gdey075T7::initialize_display_common_()
{
  ESP_LOGI(TAG, "initialize_display_common_");
  epd_spi.send_command(UC8179_CMD_POWER_SETTING);
  epd_spi.send_data(0x07); // (default) Source LV power selection: internal; Source power selection: internal; Gate power selection: internal
  epd_spi.send_data(0x07); // VGH / VGL selection: VGH = 20 V, VGL = -20 V (default)
  epd_spi.send_data(0x3F); // VDH selection: 15 V
  epd_spi.send_data(0x3F); // VDL selection: -15 V
  epd_spi.send_data(0x09); // VDHR selection: 4.2 V

  epd_spi.send_command(UC8179_CMD_RESOLUTION_SETTING);
  epd_spi.send_data(WIDTH / 256);
  epd_spi.send_data(WIDTH % 256);
  epd_spi.send_data(HEIGHT / 256);
  epd_spi.send_data(HEIGHT % 256);

  epd_spi.send_command(UC8179_CMD_DUAL_SPI);
  epd_spi.send_data(0x00); // (default) Dual SPI mode: disabled

  epd_spi.send_command(UC8179_CMD_TCON_SETTING);
  epd_spi.send_data(0x22); // Source to Gate non-overlap: 0d12; Gate to Source non-overlap: 0d12
}

/**
 * @brief Initializes normal update mode for the E-Ink display.
 */
void Gdey075T7::initialize_normal_update_()
{
  ESP_LOGI(TAG, "initialize_normal_update_");
  /*
  epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
  epd_spi.send_data(0x1F); // LUT selection: OTP; B&W mode; Gate scan direction: up; Source shift direction: right; Booster: on; Soft reset: no effect

  epd_spi.send_command(UC8179_CMD_BOOSTER_SOFT_START);
  epd_spi.send_data(0x17); // (default) Soft start phase A: 10 ms; Strength phase A: 3; Min. off time of GDR in phase A: 6.58 us
  epd_spi.send_data(0x17); // (default) Soft start phase B: 10 ms; Strength phase B: 3; Min. off time of GDR in phase B: 6.58 us
  epd_spi.send_data(0x28); // Strength phase C1: 6; Min. off time of GDR in phase C1: 0.27 us
  epd_spi.send_data(0x17); // (default) Booster phase C2: disabled; Strength phase C2: 3; Min. off time of GDR in phase C2: 6.58 us

  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0x10); // TODO: find out which value is better (CalEPD 0x10 vs. GxEPD2 0x29 vs. GD 0xA9)
  epd_spi.send_data(0x07); // VCOM and data interval: 0d10

  epd_spi.send_command(UC8179_CMD_POWER_SAVING);
  epd_spi.send_data(0x22); // VCOM Power Saving: 0d2 line period; Source power saving: 2*660 ns

  epd_spi.send_command(UC8179_CMD_CASCADE_SETTING);
  epd_spi.send_data(0x00); // Temperature value is defined by internal temperature sensor
  epd_spi.send_command(UC8179_CMD_TEMPERATURE_SENSOR_SELECTION);
  epd_spi.send_data(0x00); // Enable internal temperature sensor
  */

  epd_spi.send_command(UC8179_CMD_POWER_SETTING);
  epd_spi.send_data(0x07); // (default) Source LV power selection: internal; Source power selection: internal; Gate power selection: internal
  epd_spi.send_data(0x07); // VGH / VGL selection: VGH = 20 V, VGL = -20 V (default)
  epd_spi.send_data(0x3F); // VDH selection: 15 V
  epd_spi.send_data(0x3F); // VDL selection: -15 V

  epd_spi.send_command(UC8179_CMD_BOOSTER_SOFT_START);
  epd_spi.send_data(0x17); // (default) Soft start phase A: 10 ms; Strength phase A: 3; Min. off time of GDR in phase A: 6.58 us
  epd_spi.send_data(0x17); // (default) Soft start phase B: 10 ms; Strength phase B: 3; Min. off time of GDR in phase B: 6.58 us
  epd_spi.send_data(0x28); // Strength phase C1: 6; Min. off time of GDR in phase C1: 0.27 us
  epd_spi.send_data(0x17); // (default) Booster phase C2: disabled; Strength phase C2: 3; Min. off time of GDR in phase C2: 6.58 us

  power_on_();

  epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
  epd_spi.send_data(0x1F); // LUT selection: OTP; B&W mode; Gate scan direction: up; Source shift direction: right; Booster: on; Soft reset: no effect

  epd_spi.send_command(UC8179_CMD_RESOLUTION_SETTING);
  epd_spi.send_data(WIDTH / 256);
  epd_spi.send_data(WIDTH % 256);
  epd_spi.send_data(HEIGHT / 256);
  epd_spi.send_data(HEIGHT % 256);

  epd_spi.send_command(UC8179_CMD_DUAL_SPI);
  epd_spi.send_data(0x00); // (default) Dual SPI mode: disabled

  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0x10); // TODO: find out which value is better (CalEPD 0x10 vs. GxEPD2 0x29 vs. GD 0xA9)
  epd_spi.send_data(0x07); // VCOM and data interval: 0d10

  epd_spi.send_command(UC8179_CMD_TCON_SETTING);
  epd_spi.send_data(0x22); // Source to Gate non-overlap: 0d12; Gate to Source non-overlap: 0d12

  update_mode_ = GDEY075T7_NORMAL_UPDATE;
}

/**
 * @brief Initializes fast update mode for the E-Ink display.
 *
 * This function is similar to initialize_normal_update_(), but with a different
 * booster and soft start setting for faster updates. The temperature value is
 * set to 90 degrees Celsius.
 */
void Gdey075T7::initialize_fast_update_()
{
  ESP_LOGI(TAG, "initialize_fast_update_");
  epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
  epd_spi.send_data(0x1F); // LUT selection: OTP; B&W mode; Gate scan direction: up; Source shift direction: right; Booster: on; Soft reset: no effect

  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0x29); // TODO: find out which value is better (CalEPD 0x10 vs. GxEPD2 0x29 vs. GD 0x07)
  epd_spi.send_data(0x07); // VCOM and data interval: 0d10

  epd_spi.send_command(UC8179_CMD_BOOSTER_SOFT_START);
  epd_spi.send_data(0x27); // Soft start phase A: 10 ms; Strength phase A: 5; Min. off time of GDR in phase A: 6.58 us
  epd_spi.send_data(0x27); // Soft start phase B: 10 ms; Strength phase B: 5; Min. off time of GDR in phase B: 6.58 us
  epd_spi.send_data(0x18); // Strength phase C1: 6; Min. off time of GDR in phase C1: 0.27 us
  epd_spi.send_data(0x17); // (default) Booster phase C2: disabled; Strength phase C2: 3; Min. off time of GDR in phase C2: 6.58 us

  epd_spi.send_command(UC8179_CMD_CASCADE_SETTING);
  epd_spi.send_data(0x02); // Temperature value is defined by TS_SET register
  epd_spi.send_command(UC8179_CMD_FORCE_TEMPERATURE);
  epd_spi.send_data(0x5A); // Write 5A into TS_SET register (90 degrees Celsius)

  update_mode_ = GDEY075T7_NORMAL_UPDATE;
}

/**
 * @brief Initializes the partial update mode for the E-Ink display.
 *
 * This function configures the display for partial updates by setting the appropriate
 * LUT (Look-Up Table) and temperature settings. If the fast partial update from OTP
 * is enabled, it uses predefined settings from the OTP. Otherwise, it configures
 * the display using partial update LUTs from registers and sets VCOM and data intervals.
 */
void Gdey075T7::initialize_partial_update_()
{
  ESP_LOGI(TAG, "initialize_partial_update_");
  if (use_partial_update_from_otp_)
  {
    ESP_LOGI(TAG, "use_partial_update_from_otp_");
    epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
    epd_spi.send_data(0x1F); // full update LUT from OTP

    power_on_();

    epd_spi.send_command(UC8179_CMD_CASCADE_SETTING);
    epd_spi.send_data(0x02); // Temperature value is defined by TS_SET register
    epd_spi.send_command(UC8179_CMD_FORCE_TEMPERATURE);
    epd_spi.send_data(0x6E); // Write 6E into TS_SET register (110 degrees Celsius)

    /*epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
    epd_spi.send_data(0xA9); // TODO: add comment
    epd_spi.send_data(0x07);*/
  }
  else
  {
    epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
    epd_spi.send_data(0x3f); // partial update LUT from registers
    epd_spi.send_command(UC8179_CMD_VCOM_DC_SETTING);
    epd_spi.send_data(0x30); // -2.5V same value as in OTP
    epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
    epd_spi.send_data(0x39); // LUTBD, N2OCP: copy new to old
    epd_spi.send_data(0x07);

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

  update_mode_ = GDEY075T7_PARTIAL_UPDATE;
}

/**
 * @brief Send the display refresh command to the display.
 *
 * This function first sends the display refresh command to the display and then
 * waits for 10 ms before returning. It also calls `wait_while_busy_()` to wait
 * until the display is not busy anymore.
 *
 * This function is used when the display is in deep sleep mode and needs to be
 * refreshed.
 */
void Gdey075T7::send_refresh_command_()
{
  ESP_LOGI(TAG, "send_refresh_command_");
  epd_spi.send_command(UC8179_CMD_DISPLAY_REFRESH); // display refresh
  vTaskDelay(10 / portTICK_PERIOD_MS);
  wait_while_busy_("_refresh");
}

/**
 * @brief Initialize the display to use the given update mode.
 *
 * This function first wakes up the display from deep sleep mode if it is in deep sleep.
 * Then it calls `initialize_display_common_()` to perform common initialization, and
 * it calls one of `initialize_partial_update_()`, `initialize_normal_update_()`,
 * or `initialize_fast_update_()`, depending on the value of `mode`.
 *
 * Finally, the display is powered on.
 *
 * If `mode` is not recognized, an error message is logged.
 *
 * @param mode The update mode to use, one of `GDEY075T7_PARTIAL_UPDATE`,
 * `GDEY075T7_NORMAL_UPDATE`, or `GDEY075T7_FAST_UPDATE`.
 */
void Gdey075T7::set_update_mode(GDEY075T7_UPDATE_MODE mode)
{
  ESP_LOGI(TAG, "set_update_mode: %d", mode);
  /*if (is_in_deep_sleep_)
  {*/
  ESP_LOGI(TAG, "HW reset");
  epd_spi.hardware_reset(10); // wake up from deep sleep
  is_in_deep_sleep_ = false;
  //}

  // initialize_display_common_();

  if (mode == GDEY075T7_PARTIAL_UPDATE)
    initialize_partial_update_();
  else if (mode == GDEY075T7_NORMAL_UPDATE)
    initialize_normal_update_();
  else if (mode == GDEY075T7_FAST_UPDATE)
    initialize_fast_update_();
  else
    ESP_LOGE(TAG, "Unknown update mode: %d", mode);

  // power_on_();
}

/**
 * @brief Powers on the display.
 *
 * This function sends the power on command to the display and waits until the
 * display is no longer busy. If the display is already powered on, this function
 * does nothing.
 */
void Gdey075T7::power_on_()
{
  ESP_LOGI(TAG, "power_on_");
  /*if (is_power_on_)
  {
    ESP_LOGI(TAG, "power is already on");
    return;
  }*/

  epd_spi.send_command(UC8179_CMD_POWER_ON);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  wait_while_busy_("_PowerOn");
  is_power_on_ = true;
}

/**
 * @brief Powers off the display.
 *
 * This function sends the power off command to the display and waits until the
 * display is no longer busy. If the display is already powered off, this function
 * does nothing.
 *
 * When the display is powered off, the update mode is reset to normal update.
 */
void Gdey075T7::power_off_()
{
  ESP_LOGI(TAG, "power_off_");
  /*if (!is_power_on_)
  {
    ESP_LOGI(TAG, "power is already off");
    return;
  }*/

  epd_spi.send_command(UC8179_CMD_POWER_OFF);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  wait_while_busy_("_PowerOff");
  is_power_on_ = false;
  update_mode_ = GDEY075T7_NORMAL_UPDATE;
}

/**
 * @brief Put the display into deep sleep.
 *
 * This function sends the deep sleep command to the display and waits until the
 * display is no longer busy. The display is then put into deep sleep mode.
 *
 * This function is called automatically by `update()`.
 */
void Gdey075T7::sleep_()
{
  ESP_LOGI(TAG, "sleep_");
  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0xF7);

  power_off_();

  epd_spi.send_command(UC8179_CMD_DEEP_SLEEP);
  epd_spi.send_data(0xA5); // Magic number to enter deep sleep as defined in UC8179 datasheet

  is_in_deep_sleep_ = true;
}

/**
 * @brief Triggers a refresh operation.
 *
 * This function is called automatically by `update()`. It checks the current
 * update mode and calls either `refresh_(0, 0, WIDTH, HEIGHT)` for partial
 * updates or `send_refresh_command_()` for full updates.
 */
void Gdey075T7::refresh_()
{
  ESP_LOGI(TAG, "refresh_");
  if (update_mode_ == GDEY075T7_PARTIAL_UPDATE)
  {
    // refresh_(0, 0, WIDTH, HEIGHT);
    update_partial_();
  }
  else
  {
    // transfer_buffer_(UC8179_CMD_DATA_START_TRANSMISSION_2);
    send_refresh_command_();
  }
}

/**
 * @brief Refreshes a specified area of the E-Ink display.
 *
 * This function updates a defined rectangular section of the display.
 * It checks if an initial full screen update has been performed, and if not,
 * calls the full refresh function first. It calculates the intersection of the
 * specified area with the screen limits and adjusts the width to ensure it
 * is a multiple of 8.
 *
 * If the update mode is not partial,
 * it initializes the partial update mode. It executes the partial update by
 * sending the appropriate commands to the display driver.
 *
 * @param x The x-coordinate of the upper-left corner of the area to refresh.
 * @param y The y-coordinate of the upper-left corner of the area to refresh.
 * @param w The width of the area to refresh.
 * @param h The height of the area to refresh.
 */
void Gdey075T7::refresh_(int16_t x, int16_t y, int16_t w, int16_t h)
{
  // intersection with screen
  int16_t w1 = x < 0 ? w + x : w;
  int16_t h1 = y < 0 ? h + y : h;
  int16_t x1 = x < 0 ? 0 : x;
  int16_t y1 = y < 0 ? 0 : y;
  w1 = x1 + w1 < int16_t(WIDTH) ? w1 : int16_t(WIDTH) - x1;
  h1 = y1 + h1 < int16_t(HEIGHT) ? h1 : int16_t(HEIGHT) - y1;
  if ((w1 <= 0) || (h1 <= 0))
  {
    return;
  }

  // make x1, w1 multiple of 8
  w1 += x1 % 8;
  if (w1 % 8 > 0)
  {
    w1 += 8 - w1 % 8;
  }
  x1 -= x1 % 8;

  epd_spi.send_command(UC8179_CMD_PARTIAL_IN);
  set_partial_ram_area_(x1, y1, w1, h1);
  send_refresh_command_();
  epd_spi.send_command(UC8179_CMD_PARTIAL_OUT);
}

/**
 * @brief Set the partial update window on the display driver.
 *
 * Set the partial update window on the display driver. The window is defined
 * by the top-left and bottom-right coordinates (x, y) and the width and height
 * parameters. The x and w parameters are adjusted to ensure that the left
 * boundary is a multiple of 8 and that the right boundary is inclusive of the
 * last byte.
 *
 * @param x The x-coordinate of the top-left corner of the window.
 * @param y The y-coordinate of the top-left corner of the window.
 * @param w The width of the window.
 * @param h The height of the window.
 */
void Gdey075T7::set_partial_ram_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  ESP_LOGI(TAG, "set_partial_ram_area_");
  uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
  uint16_t ye = y + h - 1;
  x &= 0xFFF8; // byte boundary

  epd_spi.send_command(UC8179_CMD_PARTIAL_WINDOW);
  epd_spi.send_data(x / 256);
  epd_spi.send_data(x % 256);
  epd_spi.send_data(xe / 256);
  epd_spi.send_data(xe % 256 - 1);
  epd_spi.send_data(y / 256);
  epd_spi.send_data(y % 256);
  epd_spi.send_data(ye / 256);
  epd_spi.send_data(ye % 256 - 1);
  epd_spi.send_data(0x01); // Needed for full-screen partial update
}

void Gdey075T7::update_partial_()
{
  ESP_LOGI(TAG, "update_partial_");
  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0xA9); // TODO: add comment
  epd_spi.send_data(0x07);

  _buffer[10000] = 0x00;
  _buffer[10001] = 0xFF;

  epd_spi.send_command(UC8179_CMD_PARTIAL_IN);
  set_partial_ram_area_(0, 0, WIDTH, HEIGHT);
  transfer_buffer_(UC8179_CMD_DATA_START_TRANSMISSION_2);
  send_refresh_command_();
}

void Gdey075T7::clear_screen(void)
{
  ESP_LOGI(TAG, "clear_screen");
  EPD_Init();
  EPD_WhiteScreen_White_Basemap();
}

void Gdey075T7::EPD_Init(void)
{
  ESP_LOGI(TAG, "EPD_Init");
  epd_spi.hardware_reset(10);

  epd_spi.send_command(UC8179_CMD_POWER_SETTING);
  epd_spi.send_data(0x07);
  epd_spi.send_data(0x07); // VGH=20V,VGL=-20V
  epd_spi.send_data(0x3f); // VDH=15V
  epd_spi.send_data(0x3f); // VDL=-15V

  epd_spi.send_command(UC8179_CMD_BOOSTER_SOFT_START);
  epd_spi.send_data(0x17);
  epd_spi.send_data(0x17);
  epd_spi.send_data(0x28);
  epd_spi.send_data(0x17);

  power_on_();

  epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
  epd_spi.send_data(0x1F); // KW-3f   KWR-2F BWROTP 0f BWOTP 1f

  epd_spi.send_command(UC8179_CMD_RESOLUTION_SETTING);
  epd_spi.send_data(0x03); // source 800
  epd_spi.send_data(0x20);
  epd_spi.send_data(0x01); // gate 480
  epd_spi.send_data(0xE0);

  epd_spi.send_command(UC8179_CMD_DUAL_SPI);
  epd_spi.send_data(0x00);

  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0x10);
  epd_spi.send_data(0x07);

  epd_spi.send_command(UC8179_CMD_TCON_SETTING);
  epd_spi.send_data(0x22);
}

void Gdey075T7::EPD_WhiteScreen_White_Basemap(void)
{
  ESP_LOGI(TAG, "EPD_WhiteScreen_White_Basemap");
  unsigned int i;

  epd_spi.send_command(UC8179_CMD_DATA_START_TRANSMISSION_1);
  for (i = 0; i < WIDTH * HEIGHT / 8; i++)
  {
    epd_spi.send_data(EPD_BLACK);
  }
  epd_spi.send_command(UC8179_CMD_DATA_START_TRANSMISSION_2);
  for (i = 0; i < WIDTH * HEIGHT / 8; i++)
  {
    epd_spi.send_data(EPD_WHITE);
  }

  send_refresh_command_();
}

void Gdey075T7::EPD_Init_Part(void)
{
  ESP_LOGI(TAG, "EPD_Init_Part");
  epd_spi.hardware_reset(10);

  epd_spi.send_command(UC8179_CMD_PANEL_SETTING);
  epd_spi.send_data(0x1F);

  power_on_();

  epd_spi.send_command(UC8179_CMD_CASCADE_SETTING);
  epd_spi.send_data(0x02);
  epd_spi.send_command(UC8179_CMD_FORCE_TEMPERATURE);
  epd_spi.send_data(0x6E);
}

void Gdey075T7::EPD_Dis_PartAll(const unsigned char *datas)
{
  ESP_LOGI(TAG, "EPD_Dis_PartAll");
  unsigned int i;
  unsigned int x_start = 0, y_start = 0, x_end, y_end;
  unsigned int PART_COLUMN = HEIGHT, PART_LINE = WIDTH;

  x_end = x_start + PART_LINE - 1;
  y_end = y_start + PART_COLUMN - 1;

  epd_spi.send_command(UC8179_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  epd_spi.send_data(0xA9);
  epd_spi.send_data(0x07);

  epd_spi.send_command(UC8179_CMD_PARTIAL_IN);
  epd_spi.send_command(UC8179_CMD_PARTIAL_WINDOW);
  epd_spi.send_data(x_start / 256);
  epd_spi.send_data(x_start % 256); // x-start

  epd_spi.send_data(x_end / 256);
  epd_spi.send_data(x_end % 256 - 1); // x-end

  epd_spi.send_data(y_start / 256);
  epd_spi.send_data(y_start % 256); // y-start

  epd_spi.send_data(y_end / 256);
  epd_spi.send_data(y_end % 256 - 1); // y-end
  epd_spi.send_data(0x01);

  epd_spi.send_command(UC8179_CMD_DATA_START_TRANSMISSION_2);
  for (i = 0; i < PART_COLUMN * PART_LINE / 8; i++)
  {
    epd_spi.send_data(~datas[i]);
  }

  send_refresh_command_();
  sleep_();
}
