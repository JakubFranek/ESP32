#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#define LED_PIN 27
#define FAN_SWITCH_PIN 32

void task_toggle_led(void *pvParameters);
void task_toggle_fan(void *pvParameters);

void app_main(void)
{
    xTaskCreatePinnedToCore(task_toggle_led, "Toggle LED", 2048, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_toggle_fan, "Toggle fan", 2048, NULL, 1, NULL, APP_CPU_NUM);

    // no while-loop needed, as FreeRTOS task scheduler has been already started within esp-idf/components/app_startup.c
}

void task_toggle_led(void *pvParameters)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(LED_PIN, GPIO_FLOATING);

    while (1)
    {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        gpio_set_level(LED_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void task_toggle_fan(void *pvParameters)
{
    gpio_reset_pin(FAN_SWITCH_PIN);
    gpio_set_direction(FAN_SWITCH_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(FAN_SWITCH_PIN, GPIO_FLOATING);

    while (1)
    {
        gpio_set_level(FAN_SWITCH_PIN, 1);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        gpio_set_level(FAN_SWITCH_PIN, 0);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}