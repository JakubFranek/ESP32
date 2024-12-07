/*
 * This file is based on source code originally from martinberlin/CalEPD GitHub repository,
 * available at https://github.com/martinberlin/CalEPD.
 *
 * Modifications have been made to the original code by Jakub Franek (https://github.com/JakubFranek),
 * as permitted under the Apache License, Version 2.0.
 */

#ifndef epd_h
#define epd_h

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "Adafruit_GFX.h"
#include "epdspi.h"

using namespace std;

class Epd : public virtual Adafruit_GFX
{
public:
    Epd(int16_t w, int16_t h) : Adafruit_GFX(w, h) {};

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0; // Override GFX own drawPixel method
    virtual void initialize() = 0;
    virtual void update() = 0;

    size_t write(uint8_t);
    void print(const std::string &text);
    void print(const char c);
    void println(const std::string &text);
    void printerf(const char *format, ...);
    void newline();
    void draw_centered_text(const GFXfont *font, int16_t x, int16_t y, uint16_t w, uint16_t h, const char *format, ...);

protected:
    static constexpr const char *TAG = "Epd class";

    static inline uint16_t gx_uint16_min(uint16_t a, uint16_t b) { return (a < b ? a : b); };
    static inline uint16_t gx_uint16_max(uint16_t a, uint16_t b) { return (a > b ? a : b); };
    bool _using_partial_mode = false;

    template <typename T>
    static inline void
    swap(T &a, T &b)
    {
        T t = a;
        a = b;
        b = t;
    }

private:
    virtual void _wakeUp() = 0;
    virtual void _sleep() = 0;
    virtual void _waitBusy(const char *message) = 0;

    uint8_t _unicodeEasy(uint8_t c);
};
#endif
