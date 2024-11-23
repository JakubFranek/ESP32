#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "scd4x.h"

#define LED_PIN 27

void task_toggle_led(void *pvParameters);
void task_scd4x(void *pvParameters);
int8_t scd4x_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length);
int8_t scd4x_i2c_read(uint8_t address, uint8_t *payload, uint8_t length);
int8_t scd4x_delay_ms(uint16_t ms);
void print_scd4x_data(Scd4xData *data);

i2c_master_bus_handle_t bus;
i2c_master_bus_config_t i2c_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 1,
    .trans_queue_depth = 0};
i2c_device_config_t scd4x_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = SCD4X_I2C_ADDRESS,
    .scl_speed_hz = 100000,
    .scl_wait_us = 0};
i2c_master_dev_handle_t scd4x_device_handle;

Scd4xDevice scd4x_device = {
    .i2c_write = &scd4x_i2c_write,
    .i2c_read = &scd4x_i2c_read,
    .delay_ms = &scd4x_delay_ms};
Scd4xStatus scd4x_status;
Scd4xData scd4x_data;
bool scd4x_data_ready = false;
uint64_t serial_number;

void app_main(void)
{
    xTaskCreatePinnedToCore(task_toggle_led, "Toggle LED", 2048, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_scd4x, "Scd4x", 4096, NULL, 1, NULL, APP_CPU_NUM);

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

void task_scd4x(void *pvParameters)
{
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &bus));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &scd4x_config, &scd4x_device_handle));

    scd4x_status = scd4x_get_serial_number(&scd4x_device, serial_number);
    printf("[SCD4X] Read Serial Number, serial number = %lld, status = %d\n", serial_number, scd4x_status);

    scd4x_status = scd4x_perform_self_test(&scd4x_device);
    printf("[SCD4X] Perform Self Test, status = %d\n", scd4x_status);

    scd4x_status = scd4x_start_periodic_measurement(&scd4x_device);
    printf("[SCD4X] Start Measurement, status = %d\n", scd4x_status);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    while (1)
    {
        TickType_t x_last_wake_time = xTaskGetTickCount();
        printf("[SCD4X task] Free stack: %d bytes\n", (int)uxTaskGetStackHighWaterMark2(NULL));

        while (!scd4x_data_ready)
        {
            vTaskDelay(1 / portTICK_PERIOD_MS);
            scd4x_status = scd4x_get_data_ready_status(&scd4x_device, &scd4x_data_ready);
            printf("[SCD4X] Read Data Ready Status, data ready = %d, status = %d\n", scd4x_data_ready, scd4x_status);
        }

        scd4x_status = scd4x_read_measurement(&scd4x_device, &scd4x_data);
        printf("[SCD4X] Read Data, status = %d\n", scd4x_status);
        print_scd4x_data(&scd4x_data);
        scd4x_data_ready = false;

        xTaskDelayUntil(&x_last_wake_time, 5000 / portTICK_PERIOD_MS);
    }
}

int8_t scd4x_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_transmit(scd4x_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}

int8_t scd4x_i2c_read(uint8_t address, uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_receive(scd4x_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}

void print_scd4x_data(Scd4xData *data)
{
    printf("CO_2 concentration = %d ppm\n", data->co2_ppm);
    printf("Temperature = %f ug/m^3\n", data->temperature / 100.0);
    printf("Relative humidity = %f ug/m^3\n", data->relative_humidity / 100.0);
}

int8_t scd4x_delay_ms(uint16_t ms)
{
    vTaskDelay(ms);
    return 0;
}