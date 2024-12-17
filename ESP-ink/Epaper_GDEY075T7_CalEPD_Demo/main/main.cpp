/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"

#include "gfxfont.h"
#include "Fonts/FreeSansBold10pt7b.h"
#include "Fonts/FreeSansBold11pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSansBold18pt7b.h"
#include "Fonts/FreeSans10pt7b.h"
#include "Fonts/FreeSans11pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans14pt7b.h"
#include "Fonts/FreeSans15pt7b.h"
#include "Fonts/FreeSans16pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSansBold72pt7b.h"

#include "displays/goodisplay/gdey075T7.h"

#include "weather_icons.h"
#include "time_of_day_icons.h"

#define SHOW_DEBUG_RECTS false

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

    const uint8_t *weather_icon = weather_01d;

    uint32_t display_counter = 0;
    string counter_string;

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
        display.draw_aligned_text(&FreeSansBold72pt7b, 0, 0, GDEY075T7_WIDTH / 2, 107, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_CENTER, "23:45");

        display.draw_aligned_text(&FreeSansBold18pt7b, 400, 0, GDEY075T7_WIDTH / 2, 110 / 2, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "pondeli 16.12.2024");
        display.draw_aligned_text(&FreeSansBold18pt7b, 400, 110 / 2, GDEY075T7_WIDTH / 2, 110 / 2, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "Albina");

        display.drawLine(0, 110, GDEY075T7_WIDTH, 110, EPD_BLACK);
        display.drawLine(GDEY075T7_WIDTH / 2, 110, GDEY075T7_WIDTH / 2, GDEY075T7_HEIGHT, EPD_BLACK);

        display.draw_aligned_text(&FreeSansBold10pt7b, 0, 110, 100, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Teplota");
        display.draw_aligned_text(&FreeSansBold10pt7b, 50, 110, 84, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "Real.");
        display.draw_aligned_text(&FreeSansBold10pt7b, 134, 110, 84, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "Pocit.");

        display.drawBitmap(0, 145 + 0 * 48, morning, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
        display.drawBitmap(0, 145 + 1 * 48, day, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
        display.drawBitmap(0, 145 + 2 * 48, evening, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
        display.drawBitmap(0, 145 + 3 * 48, night, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
        if (SHOW_DEBUG_RECTS)
        {
            display.drawRect(0, 145 + 0 * 48, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
            display.drawRect(0, 145 + 1 * 48, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
            display.drawRect(0, 145 + 2 * 48, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
            display.drawRect(0, 145 + 3 * 48, TIME_OF_DAY_ICON_SIZE, TIME_OF_DAY_ICON_SIZE, EPD_BLACK);
        }

        display.draw_aligned_text(&FreeSans15pt7b, 50, 145 + 0 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "9.7");
        display.draw_aligned_text(&FreeSans15pt7b, 50, 145 + 1 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "23.9");
        display.draw_aligned_text(&FreeSans15pt7b, 50, 145 + 2 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "33.2");
        display.draw_aligned_text(&FreeSans15pt7b, 50, 145 + 3 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "-15.8");

        display.draw_aligned_text(&FreeSans15pt7b, 134, 145 + 0 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "8.3");
        display.draw_aligned_text(&FreeSans15pt7b, 134, 145 + 1 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "18.9");
        display.draw_aligned_text(&FreeSans15pt7b, 134, 145 + 2 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "36.4");
        display.draw_aligned_text(&FreeSans15pt7b, 134, 145 + 3 * 48, 84, 48, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "-17.5");

        display.drawLine(0, 110 + 8 * 30, 400, 110 + 8 * 30, EPD_BLACK);

        display.draw_aligned_text(&FreeSans10pt7b, 0, 110 + 8 * 30 + 4 + 0 * 25, 200, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Vlhkost: 84 %%");
        display.draw_aligned_text(&FreeSans10pt7b, 200, 110 + 8 * 30 + 4 + 0 * 25, 200, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "%% srazek: 63 %%");
        display.draw_aligned_text(&FreeSans10pt7b, 0, 110 + 8 * 30 + 4 + 1 * 25, 200, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Dest: 2.7 mm");
        display.draw_aligned_text(&FreeSans10pt7b, 200, 110 + 8 * 30 + 4 + 1 * 25, 200, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Snih: 0.0 mm");
        display.draw_aligned_text(&FreeSans10pt7b, 0, 110 + 8 * 30 + 4 + 2 * 25, 200, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Vitr: 6.1 / 11.9 m/s");
        display.draw_aligned_text(&FreeSans10pt7b, 200, 110 + 8 * 30 + 4 + 2 * 25, 200, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Oblacnost: 71 %%");

        display.drawLine(0, 429, 400, 429, EPD_BLACK);

        display.draw_aligned_text(&FreeSans10pt7b, 0, 432, 400, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "You can expect light showers in the morning,");
        display.draw_aligned_text(&FreeSans10pt7b, 0, 457, 400, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "with clearing in the afternoon.");

        display.draw_aligned_text(&FreeSansBold12pt7b, 405, 110, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Misto");
        display.draw_aligned_text(&FreeSansBold12pt7b, 535, 110, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "Teplota");
        display.draw_aligned_text(&FreeSansBold12pt7b, 670, 110, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "Vlhkost");
        display.draw_aligned_text(&FreeSans12pt7b, 405, 110 + 1 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Obyvak");
        display.draw_aligned_text(&FreeSans12pt7b, 535, 110 + 1 * 30, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "23.2 *C");
        display.draw_aligned_text(&FreeSans12pt7b, 670, 110 + 1 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "57.8 %%");
        display.draw_aligned_text(&FreeSans12pt7b, 405, 110 + 2 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Pracovna");
        display.draw_aligned_text(&FreeSans12pt7b, 535, 110 + 2 * 30, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "22.1 *C");
        display.draw_aligned_text(&FreeSans12pt7b, 670, 110 + 2 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "54.6 %%");
        display.draw_aligned_text(&FreeSans12pt7b, 405, 110 + 3 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Loznice");
        display.draw_aligned_text(&FreeSans12pt7b, 535, 110 + 3 * 30, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "19.5 *C");
        display.draw_aligned_text(&FreeSans12pt7b, 670, 110 + 3 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "59.4 %%");
        display.draw_aligned_text(&FreeSans12pt7b, 405, 110 + 4 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Kuchyn");
        display.draw_aligned_text(&FreeSans12pt7b, 535, 110 + 4 * 30, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "24.2 *C");
        display.draw_aligned_text(&FreeSans12pt7b, 670, 110 + 4 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "49.8 %%");
        display.draw_aligned_text(&FreeSans12pt7b, 405, 110 + 5 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Koupelna");
        display.draw_aligned_text(&FreeSans12pt7b, 535, 110 + 5 * 30, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "26.6 *C");
        display.draw_aligned_text(&FreeSans12pt7b, 670, 110 + 5 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "79.8 %%");
        display.draw_aligned_text(&FreeSans12pt7b, 405, 110 + 6 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Lodzie");
        display.draw_aligned_text(&FreeSans12pt7b, 535, 110 + 6 * 30, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "14.3 *C");
        display.draw_aligned_text(&FreeSans12pt7b, 670, 110 + 6 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "37.9 %%");
        display.draw_aligned_text(&FreeSans12pt7b, 405, 110 + 7 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Sever");
        display.draw_aligned_text(&FreeSans12pt7b, 535, 110 + 7 * 30, 135, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "13.3 *C");
        display.draw_aligned_text(&FreeSans12pt7b, 670, 110 + 7 * 30, 130, 30, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_RIGHT, "47.1 %%");

        display.drawLine(400, 110 + 8 * 30, 800, 110 + 8 * 30, EPD_BLACK);

        display.draw_aligned_text(&FreeSans10pt7b, 405, 110 + 8 * 30 + 4 + 0 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "CO2: 890 ppm");
        display.draw_aligned_text(&FreeSans10pt7b, 405, 110 + 8 * 30 + 4 + 1 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "VOC index: 102");
        display.draw_aligned_text(&FreeSans10pt7b, 405, 110 + 8 * 30 + 4 + 2 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "NOx index: 1");
        display.draw_aligned_text(&FreeSans10pt7b, 405, 110 + 8 * 30 + 4 + 3 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Tlak: 1029.7 hPa");

        display.draw_aligned_text(&FreeSans10pt7b, 605, 110 + 8 * 30 + 4 + 0 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "PM 1.0: 9.2 ug/m^3");
        display.draw_aligned_text(&FreeSans10pt7b, 605, 110 + 8 * 30 + 4 + 1 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "PM 2.5: 11.2 ug/m^3");
        display.draw_aligned_text(&FreeSans10pt7b, 605, 110 + 8 * 30 + 4 + 2 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "PM 10: 13.7 ug/m^3");
        display.draw_aligned_text(&FreeSans10pt7b, 605, 110 + 8 * 30 + 4 + 3 * 25, 195, 25, SHOW_DEBUG_RECTS, SHOW_DEBUG_RECTS, TEXT_ALIGNMENT_LEFT, "Typ. castice: 680 nm");

        switch (display_counter % 9)
        {
        case 0:
            weather_icon = weather_01d;
            break;
        case 1:
            weather_icon = weather_02d;
            break;
        case 2:
            weather_icon = weather_03d;
            break;
        case 3:
            weather_icon = weather_04d;
            break;
        case 4:
            weather_icon = weather_09d;
            break;
        case 5:
            weather_icon = weather_10d;
            break;
        case 6:
            weather_icon = weather_11d;
            break;
        case 7:
            weather_icon = weather_13d;
            break;
        case 8:
            weather_icon = weather_50d;
            break;
        default:
            ESP_LOGE("main", "Unknown weather icon");
        }

        display.drawBitmap(400 - 160, 169, weather_icon, WEATHER_ICON_SIZE, WEATHER_ICON_SIZE, EPD_BLACK);

        if (SHOW_DEBUG_RECTS)
            display.drawRect(400 - 160, 169, WEATHER_ICON_SIZE, WEATHER_ICON_SIZE, EPD_BLACK);

        if (display_counter > 0 && display_counter % 15 == 0)
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