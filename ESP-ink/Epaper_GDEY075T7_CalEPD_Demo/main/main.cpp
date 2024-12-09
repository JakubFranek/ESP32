/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"

#include "gfxfont.h"
#include "Fonts/FreeSansBold24pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSansBold72pt7b.h"

#include "displays/goodisplay/gdey075T7.h"

#define TEST_TEXT "12:34"

EpdSpi epd_spi;
Gdey075T7 display(epd_spi);

extern "C"
{
    void app_main(void);
}

void app_main(void)
{
    // On laskakit ESPink board, the display is powered through a transistor controlled by GPIO_NUM_2
    // To power the display, GPIO_NUM_2 must be set to 1
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);

    display.initialize();
    display.setRotation(2);

    display.fillScreen(EPD_WHITE);
    display.setTextColor(EPD_BLACK);
    display.setFont(&FreeSansBold72pt7b);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(TEST_TEXT, 0, 0, &x1, &y1, &w, &h);
    uint16_t x = -x1;
    uint16_t y = -y1;
    display.setCursor(x, y);

    display.println(TEST_TEXT);
    display.drawLine(0, 100, display.width(), 100, EPD_BLACK);

    display.draw_centered_text(&FreeSans12pt7b, 0, 0, display.width(), display.height(), "CENTER");

    display.update();

    return;
}