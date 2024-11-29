/*
 * This file is based on source code originally from martinberlin/CalEPD GitHub repository,
 * available at https://github.com/martinberlin/CalEPD.
 *
 * Modifications have been made to the original code by Jakub Franek (https://github.com/JakubFranek),
 * as permitted under the Apache License, Version 2.0.
 */

#include "displays/goodisplay/gdey029T94.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#define GDEY029T94_SPI_FREQUENCY_MHZ 4
#define GDEY029T94_HW_RESET_DELAY_MS 20
#define GDEY029T94_BUSY_TIMEOUT_US 4000000 // 4 s
#define GDEY029T94_UPDATE_SEQUENCE_COLOR 0x37
#define GDEY029T94_UPDATE_SEQUENCE_MONO 0xF7
#define GDEY029T94_UPDATE_SEQUENCE_GRAY 0xC4
#define GDEY029T94_UPDATE_SEQUENCE_MONO_FAST 0xC7
#define GDEY029T94_UPDATE_SEQUENCE_MONO_PARTIAL 0xFF
#define GDEY029T94_BORDER_WAVEFORM 0x05 // Follow LUT 1

#define SSD1680_CMD_SET_DRIVER_OUTPUT_CONTROL 0x01
#define SSD1680_CMD_SET_GATE_DRIVING_VOLTAGE 0x03
#define SSD1680_CMD_SET_SOURCE_DRIVING_VOLTAGE 0x04
#define SSD1680_CMD_SET_DEEP_SLEEP_MODE 0x10
#define SSD1680_CMD_SET_DATA_ENTRY_MODE 0x11
#define SSD1680_CMD_SW_RESET 0x12
#define SSD1680_CMD_UPDATE 0x20
#define SSD1680_CMD_SET_UPDATE_RAM 0x21
#define SSD1680_CMD_SET_UPDATE_SEQUENCE 0x22
#define SSD1680_CMD_WRITE_RAM_BW 0x24
#define SSD1680_CMD_WRITE_RAM_RED 0x26
#define SSD1680_CMD_WRITE_VCOM_REGISTER 0x2C
#define SSD1680_CMD_SET_BORDER_WAVEFORM 0x3C
#define SSD1680_CMD_SET_LUT_END_OPTION 0x3F
#define SSD1680_CMD_SELECT_TEMPERATURE_SENSOR 0x18
#define SSD1680_CMD_SET_RAM_X_START_END_ADDRESS 0x44
#define SSD1680_CMD_SET_RAM_Y_START_END_ADDRESS 0x45
#define SSD1680_CMD_SET_RAM_X_COUNTER 0x4E
#define SSD1680_CMD_SET_RAM_Y_COUNTER 0x4F

// 4-grays Waveform
const epd_lut_159 Gdey029T94::lut_4_grays = {
    0x32, {0x40, 0x48, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0x48, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x48, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x20, 0x48, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xA, 0x19, 0x0, 0x3, 0x8, 0x0, 0x0, 0x14, 0x1, 0x0, 0x14, 0x1, 0x0, 0x3, 0xA, 0x3, 0x0, 0x8, 0x19, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x0, 0x0, 0x0, 0x22, 0x17, 0x41, 0x0, 0x32, 0x1C}, 159};

// Empty constructor
Gdey029T94::Gdey029T94(EpdSpi &dio) : Adafruit_GFX(GDEY029T94_WIDTH, GDEY029T94_HEIGHT),
                                      Epd(GDEY029T94_WIDTH, GDEY029T94_HEIGHT), epd_spi(dio) {};

void Gdey029T94::init(void)
{
  epd_spi.initialize(GDEY029T94_SPI_FREQUENCY_MHZ);

  epd_spi.reset(GDEY029T94_HW_RESET_DELAY_MS);
  fillScreen(EPD_WHITE);
  _mono_mode = true;
  fillScreen(EPD_WHITE);
}

void Gdey029T94::fillScreen(uint16_t color)
{
  if (_mono_mode)
  {
    uint8_t data = (color == EPD_WHITE) ? EPD_WHITE : EPD_BLACK;
    for (uint16_t x = 0; x < sizeof(_mono_buffer); x++)
    {
      _mono_buffer[x] = data;
    }
    ESP_LOGD(TAG, "fillScreen(%d) _mono_buffer len:%d\n", data, sizeof(_mono_buffer));
  }
  else
  {
    // This is to make faster black & white
    if (color == EPD_WHITE || color == EPD_BLACK)
    {
      for (uint32_t i = 0; i < GDEY029T94_BUFFER_SIZE; i++)
      {
        _buffer1[i] = (color == EPD_WHITE) ? EPD_BLACK : EPD_WHITE;
        _buffer2[i] = (color == EPD_WHITE) ? EPD_BLACK : EPD_WHITE;
      }
      return;
    }

    for (uint32_t y = 0; y < GDEY029T94_HEIGHT; y++)
    {
      for (uint32_t x = 0; x < GDEY029T94_WIDTH; x++)
      {
        drawPixel(x, y, color);
        if (x % 8 == 0)
          vTaskDelay(pdMS_TO_TICKS(2));
      }
    }
  }
}

void Gdey029T94::update()
{
  _using_partial_mode = false;
  uint64_t startTime = esp_timer_get_time();

  uint8_t xLineBytes = GDEY029T94_WIDTH / 8;
  uint8_t x1buf[xLineBytes];

  if (_mono_mode)
  {
    _wakeUp();

    epd_spi.send_command(SSD1680_CMD_WRITE_RAM_BW); // write RAM1 for black(0)/white (1)
    for (int y = GDEY029T94_HEIGHT; y >= 0; y--)
    {
      for (uint16_t x = 0; x < xLineBytes; x++)
      {
        uint16_t idx = y * xLineBytes + x;
        uint8_t data = _mono_buffer[idx];
        x1buf[x] = data; // ~ is invert

        if (x == xLineBytes - 1)
          epd_spi.send_data(x1buf, sizeof(x1buf)); // Flush the X line buffer to SPI
      }
    }
  }
  else
  {
    uint32_t i = 0;

    _wakeUpGrayMode();

    // 4 grays mode
    epd_spi.send_command(SSD1680_CMD_WRITE_RAM_BW);
    for (int y = GDEY029T94_HEIGHT; y >= 0; y--)
    {
      for (uint16_t x = 0; x < xLineBytes; x++)
      {
        uint16_t idx = y * xLineBytes + x;
        uint8_t data = i < sizeof(_buffer1) ? _buffer1[idx] : 0x00;
        x1buf[x] = data; // ~ is invert

        if (x == xLineBytes - 1)
        {
          epd_spi.send_data(x1buf, sizeof(x1buf)); // Flush the X line buffer to SPI
        }
        ++i;
      }
    }

    i = 0;
    epd_spi.send_command(SSD1680_CMD_WRITE_RAM_RED);

    for (int y = GDEY029T94_HEIGHT; y >= 0; y--)
    {
      for (uint16_t x = 0; x < xLineBytes; x++)
      {
        uint16_t idx = y * xLineBytes + x;
        uint8_t data = i < sizeof(_buffer2) ? _buffer2[idx] : 0x00;
        x1buf[x] = data; // ~ is invert

        if (x == xLineBytes - 1)
          epd_spi.send_data(x1buf, sizeof(x1buf)); // Flush the X line buffer to SPI

        ++i;
      }
    }
  }
  uint64_t endTime = esp_timer_get_time();

  epd_spi.send_command(SSD1680_CMD_SET_UPDATE_SEQUENCE);
  uint8_t update_sequence = (_mono_mode) ? GDEY029T94_UPDATE_SEQUENCE_MONO : GDEY029T94_UPDATE_SEQUENCE_GRAY;
  epd_spi.send_data(update_sequence);
  epd_spi.send_command(SSD1680_CMD_UPDATE);

  _waitBusy("update full");
  uint64_t powerOnTime = esp_timer_get_time();

  printf("\n\nSTATS (ms)\n%llu _wakeUp settings+send Buffer\n%llu _powerOn\n%llu total time in millis\n",
         (endTime - startTime) / 1000, (powerOnTime - endTime) / 1000, (powerOnTime - startTime) / 1000);

  _sleep();
}

void Gdey029T94::updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation)
{
  if (!_using_partial_mode)
  {
    _using_partial_mode = true;
    _wakeUp();
  }

  if (using_rotation)
    _rotate(x, y, w, h);

  if (x >= GDEY029T94_WIDTH)
    return;

  if (y >= GDEY029T94_HEIGHT)
    return;

  uint16_t xe = gx_uint16_min(GDEY029T94_WIDTH, x + w) - 1;
  uint16_t ye = gx_uint16_min(GDEY029T94_HEIGHT, y + h) - 1;
  uint16_t xs_d8 = x / 8;
  uint16_t xe_d8 = xe / 8;

  epd_spi.send_command(SSD1680_CMD_SW_RESET); // SWRESET
  _waitBusy("SWRESET");

  _setRamDataEntryMode(0x03);
  _SetRamArea(xs_d8, xe_d8, y % 256, y / 256, ye % 256, ye / 256); // X-source area,Y-gate area
  _SetRamPointer(xs_d8, y % 256, y / 256);                         // set ram
  _waitBusy("updateWindow I");

  epd_spi.send_command(SSD1680_CMD_SET_UPDATE_SEQUENCE);
  epd_spi.send_data(0xFF);

  epd_spi.send_command(SSD1680_CMD_WRITE_RAM_BW); // BW RAM
  // printf("Loop from ys:%d to ye:%d\n", y, ye);

  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_d8; x1 <= xe_d8; x1++)
    {
      uint16_t idx = y1 * (GDEY029T94_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_mono_buffer)) ? _mono_buffer[idx] : 0x00;
      epd_spi.send_data(data);
    }
  }

  // If I don't do this then the 2nd partial comes out gray:
  epd_spi.send_command(SSD1680_CMD_WRITE_RAM_RED); // RAM2
  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_d8; x1 <= xe_d8; x1++)
    {
      uint16_t idx = y1 * (GDEY029T94_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_mono_buffer)) ? _mono_buffer[idx] : 0x00;
      epd_spi.send_data(~data);
    }
  }

  epd_spi.send_command(SSD1680_CMD_UPDATE);
  _waitBusy("update partial");
  _sleep();
}

void Gdey029T94::_waitBusy(const char *message)
{

  ESP_LOGD(TAG, "_waitBusy for %s", message);
  int64_t time_since_boot = esp_timer_get_time();

  while (true)
  {
    if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 0)
      break;

    vTaskDelay(10 / portTICK_PERIOD_MS);

    if (esp_timer_get_time() - time_since_boot > GDEY029T94_BUSY_TIMEOUT_US)
    {
      ESP_LOGW(TAG, "Busy Timeout");
      break;
    }
  }
}

void Gdey029T94::_sleep()
{
  epd_spi.send_command(SSD1680_CMD_SET_DEEP_SLEEP_MODE); // deep sleep
  epd_spi.send_data(0x01);
}

void Gdey029T94::_rotate(uint16_t &x, uint16_t &y, uint16_t &w, uint16_t &h)
{
  switch (getRotation())
  {
  case 0:
    break;
  case 1:
    swap(x, y);
    swap(w, h);
    x = GDEY029T94_WIDTH - x - w - 1;
    break;
  case 2:
    x = GDEY029T94_WIDTH - x - w - 1;
    y = GDEY029T94_HEIGHT - y - h - 1;
    break;
  case 3:
    swap(x, y);
    swap(w, h);
    y = GDEY029T94_HEIGHT - y - h - 1;
    break;
  }
}

void Gdey029T94::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
    return;

  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
  case 0:
    break;
  case 1:
    swap(x, y);
    x = GDEY029T94_VISIBLE_WIDTH - x - 1;
    break;
  case 2:
    x = GDEY029T94_VISIBLE_WIDTH - x - 1;
    y = GDEY029T94_HEIGHT - y - 1;
    break;
  case 3:
    swap(x, y);
    y = GDEY029T94_HEIGHT - y - 1;
    break;
  }

  uint16_t i = x / 8 + y * GDEY029T94_WIDTH / 8;
  uint8_t mask = 1 << (7 - x % 8);

  if (_mono_mode)
  {
    if (color == EPD_WHITE)
    {
      _mono_buffer[i] = _mono_buffer[i] | mask;
    }
    else
    {
      _mono_buffer[i] = _mono_buffer[i] & (0xFF ^ mask);
    }
  }
  else
  {
    // 4 gray mode
    mask = 0x80 >> (x & 7);
    color >>= 6; // Color is from 0 (black) to 255 (white)

    switch (color)
    {
    case 1:
      // Dark gray
      _buffer1[i] = _buffer1[i] & (0xFF ^ mask);
      _buffer2[i] = _buffer2[i] | mask;
      break;
    case 2:
      // Light gray
      _buffer1[i] = _buffer1[i] | mask;
      _buffer2[i] = _buffer2[i] & (0xFF ^ mask);
      break;
    case 3:
      // White
      _buffer1[i] = _buffer1[i] & (0xFF ^ mask);
      _buffer2[i] = _buffer2[i] & (0xFF ^ mask);
      break;
    default:
      // Black
      _buffer1[i] = _buffer1[i] | mask;
      _buffer2[i] = _buffer2[i] | mask;
      break;
    }
  }
}

void Gdey029T94::_wakeUp()
{
  epd_spi.reset(GDEY029T94_HW_RESET_DELAY_MS);
  _waitBusy("RST reset");
  epd_spi.send_command(SSD1680_CMD_SW_RESET); // SWRESET
  _waitBusy("SWRESET");

  epd_spi.send_command(SSD1680_CMD_SET_DRIVER_OUTPUT_CONTROL); // Driver output control
  epd_spi.send_data(0x27);
  epd_spi.send_data(0x01);
  epd_spi.send_data(0x00);

  epd_spi.send_command(SSD1680_CMD_SET_DATA_ENTRY_MODE); // data entry mode
  epd_spi.send_data(0x01);

  epd_spi.send_command(SSD1680_CMD_SET_BORDER_WAVEFORM); // BorderWavefrom
  epd_spi.send_data(0x05);

  epd_spi.send_command(SSD1680_CMD_SET_UPDATE_RAM); //  Display update control
  epd_spi.send_data(0x00);
  epd_spi.send_data(0x80);

  epd_spi.send_command(SSD1680_CMD_SELECT_TEMPERATURE_SENSOR); // Read built-in temperature sensor
  epd_spi.send_data(0x80);

  epd_spi.send_command(SSD1680_CMD_SET_RAM_X_START_END_ADDRESS); // set Ram-X address start/end position
  epd_spi.send_data(0x00);
  epd_spi.send_data(0x0F); // 0x0F-->(15+1)*8=128

  epd_spi.send_command(SSD1680_CMD_SET_RAM_Y_START_END_ADDRESS); // set Ram-Y address start/end position
  epd_spi.send_data(0x27);                                       // 0x0127-->(295+1)=296
  epd_spi.send_data(0x01);
  epd_spi.send_data(0x00);
  epd_spi.send_data(0x00);
  _waitBusy("wakeup CMDs");
}

void Gdey029T94::_wakeUpGrayMode()
{
  epd_spi.reset(GDEY029T94_HW_RESET_DELAY_MS);
  _waitBusy("RST reset");
  epd_spi.send_command(SSD1680_CMD_SW_RESET); // SWRESET
  _waitBusy("SWRESET");

  epd_spi.send_command(SSD1680_CMD_SET_BORDER_WAVEFORM); // BorderWavefrom
  epd_spi.send_data(0x05);

  epd_spi.send_command(SSD1680_CMD_WRITE_VCOM_REGISTER); // VCOM Voltage
  epd_spi.send_data(lut_4_grays.data[158]);              // 0x1C

  epd_spi.send_command(SSD1680_CMD_SET_LUT_END_OPTION); // EOPQ
  epd_spi.send_data(lut_4_grays.data[153]);

  epd_spi.send_command(SSD1680_CMD_SET_GATE_DRIVING_VOLTAGE); // VGH
  epd_spi.send_data(lut_4_grays.data[154]);

  epd_spi.send_command(SSD1680_CMD_SET_SOURCE_DRIVING_VOLTAGE); // Check what is this about
  epd_spi.send_data(lut_4_grays.data[155]);                     // VSH1
  epd_spi.send_data(lut_4_grays.data[156]);                     // VSH2
  epd_spi.send_data(lut_4_grays.data[157]);                     // VSL

  // LUT init table for 4 gray. Check if it's needed!
  epd_spi.send_command(lut_4_grays.cmd); // boost
  for (int i = 0; i < lut_4_grays.databytes; ++i)
  {
    epd_spi.send_data(lut_4_grays.data[i]);
  }
}

void Gdey029T94::_SetRamArea(uint8_t Xstart, uint8_t Xend, uint8_t Ystart, uint8_t Ystart1, uint8_t Yend, uint8_t Yend1)
{

  ESP_LOGD(TAG, "_SetRamArea(xS:%d,xE:%d,Ys:%d,Y1s:%d,Ye:%d,Ye1:%d)\n", Xstart, Xend, Ystart, Ystart1, Yend, Yend1);

  epd_spi.send_command(SSD1680_CMD_SET_RAM_X_START_END_ADDRESS);
  epd_spi.send_data(Xstart);
  epd_spi.send_data(Xend);
  epd_spi.send_command(SSD1680_CMD_SET_RAM_Y_START_END_ADDRESS);
  epd_spi.send_data(Ystart);
  epd_spi.send_data(Ystart1);
  epd_spi.send_data(Yend);
  epd_spi.send_data(Yend1);
}

void Gdey029T94::_SetRamPointer(uint8_t addrX, uint8_t addrY, uint8_t addrY1)
{
  ESP_LOGD(TAG, "_SetRamPointer(addrX:%d,addrY:%d,addrY1:%d)\n", addrX, addrY, addrY1);

  epd_spi.send_command(SSD1680_CMD_SET_RAM_X_COUNTER);
  epd_spi.send_data(addrX);
  epd_spi.send_command(SSD1680_CMD_SET_RAM_Y_COUNTER);
  epd_spi.send_data(addrY);
  epd_spi.send_data(addrY1);
}

// We use only 0x03: At the moment this method is not used
// ram_entry_mode = 0x03; // y-increment, x-increment : normal mode
// ram_entry_mode = 0x00; // y-decrement, x-decrement
// ram_entry_mode = 0x01; // y-decrement, x-increment
// ram_entry_mode = 0x02; // y-increment, x-decrement
void Gdey029T94::_setRamDataEntryMode(uint8_t ram_entry_mode)
{
  const uint16_t xPixelsPar = GDEY029T94_X_PIXELS - 1;
  const uint16_t yPixelsPar = GDEY029T94_Y_PIXELS - 1;
  ram_entry_mode = gx_uint16_min(ram_entry_mode, 0x03);
  epd_spi.send_command(SSD1680_CMD_SET_DATA_ENTRY_MODE);
  epd_spi.send_data(ram_entry_mode);

  switch (ram_entry_mode)
  {
  case 0x00:                                                                           // x decrease, y decrease
    _SetRamArea(xPixelsPar / 8, 0x00, yPixelsPar % 256, yPixelsPar / 256, 0x00, 0x00); // X-source area, Y-gate area
    _SetRamPointer(xPixelsPar / 8, yPixelsPar % 256, yPixelsPar / 256);                // set ram
    break;
  case 0x01:                                                                           // x increase, y decrease : as in demo code
    _SetRamArea(0x00, xPixelsPar / 8, yPixelsPar % 256, yPixelsPar / 256, 0x00, 0x00); // X-source area, Y-gate area
    _SetRamPointer(0x00, yPixelsPar % 256, yPixelsPar / 256);                          // set ram
    break;
  case 0x02:                                                                           // x decrease, y increase
    _SetRamArea(xPixelsPar / 8, 0x00, 0x00, 0x00, yPixelsPar % 256, yPixelsPar / 256); // X-source area, Y-gate area
    _SetRamPointer(xPixelsPar / 8, 0x00, 0x00);                                        // set ram
    break;
  case 0x03:                                                                           // x increase, y increase : normal mode
    _SetRamArea(0x00, xPixelsPar / 8, 0x00, 0x00, yPixelsPar % 256, yPixelsPar / 256); // X-source area, Y-gate area
    _SetRamPointer(0x00, 0x00, 0x00);                                                  // set ram
    break;
  }
}

/**
 * @brief Sets monochrome mode flag.
 *
 * @param[in] mode true to use monochrome mode, false to use 4 gray mode.
 */
void Gdey029T94::setMonoMode(bool mode)
{
  _mono_mode = mode;
}
