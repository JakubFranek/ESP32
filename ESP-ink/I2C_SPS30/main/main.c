#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "sps30_i2c.h"

#define LED_PIN 27

void task_toggle_led(void *pvParameters);
void task_sps30(void *pvParameters);
int8_t sps30_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length);
int8_t sps30_i2c_read(uint8_t address, uint8_t *payload, uint8_t length);

i2c_master_bus_handle_t bus;
i2c_master_bus_config_t i2c_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 1,
    .trans_queue_depth = 0};
i2c_device_config_t sps30_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = SPS30_I2C_ADDRESS,
    .scl_speed_hz = 100000,
    .scl_wait_us = 0};
i2c_master_dev_handle_t sps30_device_handle;
Sps30Device sps30_device = {
    .i2c_write = &sps30_i2c_write,
    .i2c_read = &sps30_i2c_read};
Sps30Status sps30_status;
Sps30FirmwareVersion sps30_version;

void app_main(void)
{
    xTaskCreatePinnedToCore(task_toggle_led, "Toggle LED", 2048, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_sps30, "Sps30", 4096, NULL, 1, NULL, APP_CPU_NUM);

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

void task_sps30(void *pvParameters)
{
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &bus));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &sps30_config, &sps30_device_handle));

    vTaskDelay(100 / portTICK_PERIOD_MS); // max 10s conditioning*/

    while (1)
    {
        TickType_t x_last_wake_time = xTaskGetTickCount();

        printf("Free stack: %d bytes\n", (int)uxTaskGetStackHighWaterMark2(NULL));

        sps30_status = sps30_read_firmware_version(&sps30_device, &sps30_version);
        printf("[SPS30] Read Firmware Version, version = %d.%d, status = %d\n", sps30_version.major, sps30_version.minor, sps30_status);

        xTaskDelayUntil(&x_last_wake_time, 1000 / portTICK_PERIOD_MS);
    }
}

int8_t sps30_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length)
{

    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_transmit(sps30_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}

int8_t sps30_i2c_read(uint8_t address, uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_receive(sps30_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}