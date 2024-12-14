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

#define TEST_TEXT "23:45"

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
    display.clear_screen();
    display.setRotation(2);

    display.setTextColor(EPD_BLACK);
    display.setFont(&FreeSansBold72pt7b);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(TEST_TEXT, 0, 0, &x1, &y1, &w, &h);
    int16_t x = -x1 + 1;
    int16_t y = -y1 + 1;

    uint32_t display_counter = 0;
    string counter_string;
    TickType_t xLastWakeTime;

    while (true)
    {
        display.fillScreen(EPD_WHITE);
        display.setCursor(x, y);
        counter_string = std::to_string(display_counter);
        display.print(counter_string);

        if (display_counter > 0 && display_counter % 10 == 0)
        {
            display.clear_screen();
        }
        display.update();

        display_counter++;

        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);

        display.wake_up(); // this re-initializes the "old" frame buffer after deep sleep period, which prevents pixel noise
    }

    return;
}