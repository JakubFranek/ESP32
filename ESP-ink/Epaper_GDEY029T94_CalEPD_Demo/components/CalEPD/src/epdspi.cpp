/*
 * This file is based on source code originally from martinberlin/CalEPD GitHub repository,
 * available at https://github.com/martinberlin/CalEPD.
 *
 * Modifications have been made to the original code by Jakub Franek (https://github.com/JakubFranek),
 * as permitted under the Apache License, Version 2.0.
 */

#include <epdspi.h>
#include <string.h>
#include "freertos/task.h"
#include "esp_log.h"

void EpdSpi::initialize(uint8_t frequency_MHz = 4)
{
    gpio_set_direction((gpio_num_t)CONFIG_EINK_SPI_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CONFIG_EINK_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CONFIG_EINK_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CONFIG_EINK_BUSY, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)CONFIG_EINK_BUSY, GPIO_PULLUP_ONLY);

    gpio_set_level((gpio_num_t)CONFIG_EINK_SPI_CS, 1);
    gpio_set_level((gpio_num_t)CONFIG_EINK_DC, 1);
    gpio_set_level((gpio_num_t)CONFIG_EINK_RST, 1);

    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_EINK_SPI_MOSI,
        .miso_io_num = -1, // MISO not used
        .sclk_io_num = CONFIG_EINK_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };

    // Config Frequency and SS GPIO
    spi_device_interface_config_t devcfg = {
        .mode = 0, // SPI mode 0
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = frequency_MHz * 1000000,
        .input_delay_ns = 0,
        .spics_io_num = CONFIG_EINK_SPI_CS,
        .flags = (SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE),
        .queue_size = 5,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));

    ESP_LOGI(TAG, "SPI initialized at %d MHz", frequency_MHz);
}

void EpdSpi::send_command(const uint8_t cmd)
{
    ESP_LOGD(TAG, "Sending command: 0x%x", cmd);

    spi_transaction_t t;

    memset(&t, 0, sizeof(t)); // Zero out the transaction
    t.length = 8;             // Command is 8 bits
    t.tx_buffer = &cmd;       // The data is the cmd itself

    gpio_set_level((gpio_num_t)CONFIG_EINK_DC, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
    gpio_set_level((gpio_num_t)CONFIG_EINK_DC, 1);
}

void EpdSpi::send_data(uint8_t data)
{
    ESP_LOGD(TAG, "Sending data: 0x%x", data);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); // Zero out the transaction
    t.length = 8;             // Command is 8 bits
    t.tx_buffer = &data;      // The data is the cmd itself
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

void EpdSpi::send_data(const uint8_t *data, int length)
{
    if (length == 0)
        return;

    ESP_LOGD(TAG, "Sending array of data, length: %d", length);
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));                              // Zero out the transaction
    t.length = length * 8;                                 // Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;                                    // Data
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t)); // Transmit!
}

void EpdSpi::reset(uint8_t wait_ms = 20)
{
    gpio_set_level((gpio_num_t)CONFIG_EINK_RST, 0);
    vTaskDelay(wait_ms / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t)CONFIG_EINK_RST, 1);
    vTaskDelay(wait_ms / portTICK_PERIOD_MS);
}
