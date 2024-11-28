/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "gfxfont.h"
#include "Fonts/FreeSans12pt7b.h"

#include "displays/goodisplay/gdey029T94.h"

EpdSpi io;
Gdey029T94 display(io);

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

    display.init(false);
    display.setRotation(3);

    display.setMonoMode(true);
    display.fillScreen(EPD_WHITE);
    display.setTextColor(EPD_BLACK);
    display.setFont(&FreeSans12pt7b);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds("This is a test text", 0, 0, &x1, &y1, &w, &h);
    uint16_t x = -x1;
    uint16_t y = -y1;
    printf("x1=%d y1=%d w=%d h=%d x=%d y=%d\n", x1, y1, w, h, x, y);
    display.setCursor(x, y);

    display.println("This is a test text");
    display.drawLine(0, 0, display.width(), display.height(), EPD_BLACK);

    display.update();

    return;
}
