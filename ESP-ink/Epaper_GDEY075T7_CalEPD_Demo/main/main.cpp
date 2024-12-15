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

#include "image.h"

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
    int16_t x = -x1 + 4;
    int16_t y = -y1 + 1;

    uint32_t display_counter = 0;
    string counter_string;

    // Create a string containing all printable ASCII characters
    // (32 to 126 decimal, 0x20 to 0x7E hexadecimal)
    string ascii_string = "ASCII 0x20 to 0x7E:";
    for (int i = 32; i <= 126; i++)
    {
        ascii_string += char(i);
    }

    while (true)
    {
        if (display_counter > 0)
        {
            gpio_set_level(GPIO_NUM_2, 1);
            vTaskDelay(10 / portTICK_PERIOD_MS);

            display.wake_up(); // this re-initializes the "old" frame buffer after deep sleep period, which prevents pixel noise
        }

        /* --- Frame buffer CAN change now --- */
        display.fillScreen(EPD_WHITE);
        display.setCursor(x, y);
        counter_string = std::to_string(display_counter);
        display.setFont(&FreeSansBold72pt7b);
        display.print(counter_string);

        display.setCursor(x, y + 300);
        display.setFont(&FreeSans12pt7b);
        display.print(ascii_string);

        display.draw_centered_text(&FreeSans12pt7b, 400, 50, 50, 50, true, true, "g");
        display.draw_centered_text(&FreeSans12pt7b, 450 - 1, 50, 50, 50, true, true, "a");
        display.draw_centered_text(&FreeSans12pt7b, 500 - 2, 50, 50, 50, true, true, "f");
        display.draw_centered_text(&FreeSans12pt7b, 550 - 3, 50, 50, 50, true, true, "gaf");
        display.draw_centered_text(&FreeSans12pt7b, 600 - 4, 50, 100, 50, true, true, "ABC");

        display.drawBitmap(100, 100, img, 200, 200, EPD_BLACK);

        if (display_counter > 0 && display_counter % 10 == 0)
        {
            display.clear_screen();
        }
        display.update();
        /* --- Frame buffer MUST NOT change now --- */

        display_counter++;
        gpio_set_level(GPIO_NUM_2, 0);

        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
    }

    return;
}