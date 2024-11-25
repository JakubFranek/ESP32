#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "bme280_i2c.h"

#define LED_PIN 27

void task_toggle_led(void *pvParameters);
void task_bme280(void *pvParameters);
int8_t bme280_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length);
int8_t bme280_i2c_read(uint8_t address, uint8_t *payload, uint8_t length);
int8_t bme280_delay_ms(uint16_t ms);
void print_bme280_data(Bme280Data *data);

i2c_master_bus_handle_t bus;
i2c_master_bus_config_t i2c_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 1,
    .trans_queue_depth = 0};
i2c_device_config_t bme280_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BME280_I2C_ADDRESS_SDO_LOW,
    .scl_speed_hz = 100000,
    .scl_wait_us = 0};
i2c_master_dev_handle_t bme280_device_handle;

Bme280Device bme280_device = {
    .i2c_write = &bme280_i2c_write,
    .i2c_read = &bme280_i2c_read,
    .address = BME280_I2C_ADDRESS_SDO_LOW,
    .config = {
        .filter = BME280_FILTER_OFF,
        .spi_3wire_enable = false,
        .standby_time = BME280_STANDBY_TIME_0_5_MS,
        .temperature_oversampling = BME280_OVERSAMPLING_X1,
        .pressure_oversampling = BME280_OVERSAMPLING_X1,
        .humidity_oversampling = BME280_OVERSAMPLING_X1}};
Bme280Status bme280_status;
Bme280Data bme280_data;
bool bme280_measurement_in_progress = false;

void app_main(void)
{
    xTaskCreatePinnedToCore(task_toggle_led, "Toggle LED", 2048, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_bme280, "Bme280", 4096, NULL, 2, NULL, APP_CPU_NUM);

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

void task_bme280(void *pvParameters)
{
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &bus));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &bme280_config, &bme280_device_handle));

    bme280_status = bme280_reset(&bme280_device);
    printf("[BME280] Reset, status = %d\n", bme280_status);

    vTaskDelay(1 / portTICK_PERIOD_MS);

    bme280_status = bme280_init(&bme280_device);
    printf("[BME280] Init, status = %d\n", bme280_status);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (1)
    {
        TickType_t x_last_wake_time = xTaskGetTickCount();
        printf("[BME280 task] Free stack: %d bytes\n", (int)uxTaskGetStackHighWaterMark2(NULL));

        bme280_status = bme280_set_mode(&bme280_device, BME280_MODE_FORCED);
        printf("[BME280] Set Forced Mode, status = %d\n", bme280_status);

        bme280_status = bme280_is_measuring(&bme280_device, &bme280_measurement_in_progress);
        printf("[BME280] Is Measuring = %d, status = %d\n", bme280_measurement_in_progress, bme280_status);

        vTaskDelay(500 / portTICK_PERIOD_MS);

        bme280_status = bme280_read_measurement(&bme280_device, &bme280_data);
        printf("[BME280] Read Data, status = %d\n", bme280_status);
        print_bme280_data(&bme280_data);

        xTaskDelayUntil(&x_last_wake_time, 1000 / portTICK_PERIOD_MS);
    }
}

int8_t bme280_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_transmit(bme280_device_handle, payload, length, 20);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    return err == ESP_OK ? 0 : -1;
}

int8_t bme280_i2c_read(uint8_t address, uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_receive(bme280_device_handle, payload, length, 20);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    return err == ESP_OK ? 0 : -1;
}

void print_bme280_data(Bme280Data *data)
{
    printf("Temperature = %f °C\n", data->temperature / 100.0);
    printf("Relative humidity = %f %%\n", data->humidity / 1000.0);
    printf("Pressure = %f Pa\n", data->pressure / 10.0);
}

int8_t bme280_delay_ms(uint16_t ms)
{
    vTaskDelay(ms);
    return 0;
}