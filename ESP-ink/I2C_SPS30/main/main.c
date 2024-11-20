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
void print_sps30_float_data(Sps30FloatData *data);
void print_sps30_uint16_data(Sps30Uint16Data *data);

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
char product_type[8] = {'\0'};
char serial_number[32] = {'\0'};
Sps30StatusFlags sps30_flags;
Sps30FloatData sps30_float_data;
Sps30Uint16Data sps30_uint16_data;
bool sps30_data_ready = false;

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

    sps30_status = sps30_read_product_type(&sps30_device, product_type);
    printf("[SPS30] Read Product Type, product type = %s, status = %d\n", product_type, sps30_status);

    sps30_status = sps30_read_serial_number(&sps30_device, serial_number);
    printf("[SPS30] Read Serial Number, serial number = %s, status = %d\n", serial_number, sps30_status);

    sps30_status = sps30_read_firmware_version(&sps30_device, &sps30_version);
    printf("[SPS30] Read Firmware Version, version = %d.%d, status = %d\n", sps30_version.major, sps30_version.minor, sps30_status);

    sps30_status = sps30_read_device_status_flags(&sps30_device, &sps30_flags);
    printf("[SPS30] Read Device Status Flags, speed warning = %d, laser error = %d, fan error = %d, status = %d\n",
           sps30_flags.speed_warning, sps30_flags.laser_error, sps30_flags.fan_error, sps30_status);

    sps30_status = sps30_start_measurement(&sps30_device, SPS30_FLOAT);
    printf("[SPS30] Start Measurement (Float), status = %d\n", sps30_status);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1)
    {
        TickType_t x_last_wake_time = xTaskGetTickCount();

        printf("Free stack: %d bytes\n", (int)uxTaskGetStackHighWaterMark2(NULL));

        while (!sps30_data_ready)
        {
            vTaskDelay(1 / portTICK_PERIOD_MS);
            sps30_status = sps30_read_data_ready_flag(&sps30_device, &sps30_data_ready);
            printf("[SPS30] Read Data Ready Flag, data ready = %d, status = %d\n", sps30_data_ready, sps30_status);
        }

        sps30_status = sps30_read_measured_values_float(&sps30_device, &sps30_float_data);
        printf("[SPS30] Read Float Data, status = %d\n", sps30_status);
        print_sps30_float_data(&sps30_float_data);
        sps30_data_ready = false;

        xTaskDelayUntil(&x_last_wake_time, 1000 / portTICK_PERIOD_MS);

        /*sps30_status = sps30_stop_measurement(&sps30_device);
        printf("[SPS30] Stop Measurement, status = %d\n", sps30_status);

        vTaskDelay(2000 / portTICK_PERIOD_MS);

        sps30_status = sps30_start_measurement(&sps30_device, SPS30_UINT16);
        printf("[SPS30] Start Measurement (uint16), status = %d\n", sps30_status);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        while (!sps30_data_ready)
        {
            vTaskDelay(1 / portTICK_PERIOD_MS);
            sps30_status = sps30_read_data_ready_flag(&sps30_device, &sps30_data_ready);
            printf("[SPS30] Read Data Ready Flag, data ready = %d, status = %d\n", sps30_data_ready, sps30_status);
        }

        sps30_status = sps30_read_measured_values_uint16(&sps30_device, &sps30_uint16_data);
        printf("[SPS30] Read uint16 Data, status = %d\n", sps30_status);
        print_sps30_uint16_data(&sps30_uint16_data);
        sps30_data_ready = false;

        sps30_status = sps30_stop_measurement(&sps30_device);
        printf("[SPS30] Stop Measurement, status = %d\n", sps30_status);

        xTaskDelayUntil(&x_last_wake_time, 1000 / portTICK_PERIOD_MS);*/
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

void print_sps30_float_data(Sps30FloatData *data)
{
    printf("Mass concentration (PM1.0) = %f ug/m^3\n", data->mass_concentration_pm1_0);
    printf("Mass concentration (PM2.5) = %f ug/m^3\n", data->mass_concentration_pm2_5);
    printf("Mass concentration (PM4.0) = %f ug/m^3\n", data->mass_concentration_pm4_0);
    printf("Mass concentration (PM10.0) = %f ug/m^3\n", data->mass_concentration_pm10_0);
    printf("Number concentration (PM0.5) = %f #/cm^3\n", data->number_concentration_pm0_5);
    printf("Number concentration (PM1.0) = %f #/cm^3\n", data->number_concentration_pm1_0);
    printf("Number concentration (PM2.5) = %f #/cm^3\n", data->number_concentration_pm2_5);
    printf("Number concentration (PM4.0) = %f #/cm^3\n", data->number_concentration_pm4_0);
    printf("Number concentration (PM10.0) = %f #/cm^3\n", data->number_concentration_pm10_0);
    printf("Typical particle size = %f nm\n", data->typical_particle_size);
}

void print_sps30_uint16_data(Sps30Uint16Data *data)
{
    printf("Mass concentration (PM1.0) = %d ug/m^3\n", data->mass_concentration_pm1_0);
    printf("Mass concentration (PM2.5) = %d ug/m^3\n", data->mass_concentration_pm2_5);
    printf("Mass concentration (PM4.0) = %d ug/m^3\n", data->mass_concentration_pm4_0);
    printf("Mass concentration (PM10.0) = %d ug/m^3\n", data->mass_concentration_pm10_0);
    printf("Number concentration (PM0.5) = %d #/cm^3\n", data->number_concentration_pm0_5);
    printf("Number concentration (PM1.0) = %d #/cm^3\n", data->number_concentration_pm1_0);
    printf("Number concentration (PM2.5) = %d #/cm^3\n", data->number_concentration_pm2_5);
    printf("Number concentration (PM4.0) = %d #/cm^3\n", data->number_concentration_pm4_0);
    printf("Number concentration (PM10.0) = %d #/cm^3\n", data->number_concentration_pm10_0);
    printf("Typical particle size = %d nm\n", data->typical_particle_size);
}