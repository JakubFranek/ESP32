#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "sht4x.h"

#define LED_PIN 27

void task_toggle_led(void *pvParameters);
void task_sht4x(void *pvParameters);
int8_t sht4x_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length);
int8_t sht4x_i2c_read(uint8_t address, uint8_t *payload, uint8_t length);
void print_sht4x_data(Sht4xData *data);

i2c_master_bus_handle_t bus;
i2c_master_bus_config_t i2c_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 1,
    .trans_queue_depth = 0};
i2c_device_config_t sht4x_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = SHT4X_I2C_ADDR_A,
    .scl_speed_hz = 100000,
    .scl_wait_us = 0};
i2c_master_dev_handle_t sht4x_device_handle;

Sht4xDevice sht4x_device = {
    .i2c_write = &sht4x_i2c_write,
    .i2c_read = &sht4x_i2c_read,
    .i2c_address = SHT4X_I2C_ADDR_A,
};
Sht4xStatus sht4x_status;
Sht4xData sht4x_data;
uint32_t serial_number;

void app_main(void)
{
    xTaskCreatePinnedToCore(task_toggle_led, "Toggle LED", 2048, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_sht4x, "Sht4x", 4096, NULL, 1, NULL, APP_CPU_NUM);

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

void task_sht4x(void *pvParameters)
{
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &bus));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &sht4x_config, &sht4x_device_handle));

    sht4x_status = sht4x_read_serial_number(&sht4x_device, &serial_number);
    printf("[SHT4X] Read Serial Number, serial number = %ld, status = %d\n", serial_number, sht4x_status);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1)
    {
        TickType_t x_last_wake_time = xTaskGetTickCount();
        printf("[SHT4X task] Free stack: %d bytes\n", (int)uxTaskGetStackHighWaterMark2(NULL));

        sht4x_status = sht4x_start_measurement(&sht4x_device, SHT4X_I2C_CMD_MEAS_HIGH_PREC);

        xTaskDelayUntil(&x_last_wake_time, 10 / portTICK_PERIOD_MS);

        sht4x_status = sht4x_read_measurement(&sht4x_device, &sht4x_data);
        printf("[SHT4X] Read Data, status = %d\n", sht4x_status);
        print_sht4x_data(&sht4x_data);

        xTaskDelayUntil(&x_last_wake_time, 1000 / portTICK_PERIOD_MS);
    }
}

int8_t sht4x_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_transmit(sht4x_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}

int8_t sht4x_i2c_read(uint8_t address, uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_receive(sht4x_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}

void print_sht4x_data(Sht4xData *data)
{
    printf("Temperature = %f °C\n", data->temperature / 1000.0);
    printf("Temperature = %f %%\n", data->humidity / 1000.0);
}