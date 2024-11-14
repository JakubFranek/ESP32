#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "sgp41.h"

#define LED_PIN 27

void task_toggle_led(void *pvParameters);
void task_get_sgp41_serial_number(void *pvParameters);

void app_main(void)
{
    printf("Setup done");

    xTaskCreatePinnedToCore(task_toggle_led, "Toggle LED", 2048, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_get_sgp41_serial_number, "Get SGP41 serial number", 4 * 2048, NULL, 1, NULL, APP_CPU_NUM);

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
        vTaskDelay(250 / portTICK_PERIOD_MS);

        gpio_set_level(LED_PIN, 0);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void task_get_sgp41_serial_number(void *pvParameters)
{
    i2c_master_bus_handle_t bus;
    i2c_master_bus_config_t i2c_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 1,
        .trans_queue_depth = 0};
    i2c_device_config_t sgp41_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SGP41_I2C_ADDR,
        .scl_speed_hz = 100000,
        .scl_wait_us = 0};
    i2c_master_dev_handle_t sgp41_device_handle;

    uint8_t sgp41_serial_number[SGP41_RSP_LEN_SERIAL_NO];

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &bus));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &sgp41_config, &sgp41_device_handle));

    while (1)
    {
        // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
        ESP_ERROR_CHECK(i2c_master_transmit(sgp41_device_handle, (uint8_t[])SGP41_CMD_SERIAL_NO, SGP41_CMD_LEN, 10));
        ESP_ERROR_CHECK(i2c_master_receive(sgp41_device_handle, sgp41_serial_number, SGP41_RSP_LEN_SERIAL_NO, 10));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}