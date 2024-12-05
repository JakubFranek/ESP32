#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "sgp41.h"

#define LED_PIN 27

void task_toggle_led(void *pvParameters);
void task_measure_voc_nox(void *pvParameters);
int8_t sgp41_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length);
int8_t sgp41_i2c_read(uint8_t address, uint8_t *payload, uint8_t length);

i2c_master_bus_handle_t bus;
i2c_master_bus_config_t i2c_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 1,
    .trans_queue_depth = 0,
};
i2c_device_config_t sgp41_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = SGP41_I2C_ADDRESS,
    .scl_speed_hz = 100000,
    .scl_wait_us = 0,
};
i2c_master_dev_handle_t sgp41_device_handle;

Sgp41Device sgp41_device = {
    .i2c_write = &sgp41_i2c_write,
    .i2c_read = &sgp41_i2c_read,
};
uint64_t sgp41_serial_number;
Sgp41Status sgp41_status;
Sgp41Data sgp41_data;

void app_main(void)
{
    printf("Setup done");

    xTaskCreatePinnedToCore(task_toggle_led, "Toggle LED", 2048, NULL, 1, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(task_measure_voc_nox, "Measure VOC and NOx", 4096, NULL, 1, NULL, APP_CPU_NUM);

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

void task_measure_voc_nox(void *pvParameters)
{
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &bus));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &sgp41_config, &sgp41_device_handle));

    sgp41_status = sgp41_initialize(&sgp41_device);

    sgp41_status = sgp41_get_serial_number(&sgp41_device, &sgp41_serial_number);
    printf("[SGP41] Get Serial Number, status = %d, serial number: %lld\n",
           sgp41_status, sgp41_serial_number);

    sgp41_status = sgp41_execute_self_test(&sgp41_device);
    printf("[SGP41] Execute Self Test, status = %d\n", sgp41_status);
    vTaskDelay(350 / portTICK_PERIOD_MS);
    sgp41_status = sgp41_evaluate_self_test(&sgp41_device);
    printf("[SGP41] Evaluate Self Test, status = %d\n", sgp41_status);

    sgp41_status = sgp41_execute_conditioning(&sgp41_device);
    printf("[SGP41] Execute Conditioning, status = %d\n", sgp41_status);

    if (sgp41_status != SGP41_SUCCESS)
    {
        sgp41_status = sgp41_turn_heater_off(&sgp41_device);
        printf("[SGP41] Turn Heater Off, status = %d\n", sgp41_status);
        return;
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS); // max 10s conditioning*/

    while (1)
    {
        TickType_t x_last_wake_time = xTaskGetTickCount();

        printf("Free stack: %d bytes\n", (int)uxTaskGetStackHighWaterMark2(NULL));

        sgp41_status = sgp41_measure_raw_signals(&sgp41_device, NULL, NULL);
        printf("[SGP41] Measure Raw Signals, status = %d\n", sgp41_status);
        vTaskDelay(55 / portTICK_PERIOD_MS);
        sgp41_status = sgp41_read_gas_indices(&sgp41_device, &sgp41_data);
        printf("[SGP41] Read Gas Indices, status = %d, voc = %d, nox = %d\n", sgp41_status, (int)sgp41_data.voc_index, (int)sgp41_data.nox_index);

        xTaskDelayUntil(&x_last_wake_time, 1000 / portTICK_PERIOD_MS);
    }
}

int8_t sgp41_i2c_write(uint8_t address, const uint8_t *payload, uint8_t length)
{

    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_transmit(sgp41_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}

int8_t sgp41_i2c_read(uint8_t address, uint8_t *payload, uint8_t length)
{
    (void)address; // Address not necessary
    // For unknown reason the timeout has to be quite long (5 ms is not enough, 10 ms seems to be fine)
    esp_err_t err = i2c_master_receive(sgp41_device_handle, payload, length, 10);
    return err == ESP_OK ? 0 : -1;
}