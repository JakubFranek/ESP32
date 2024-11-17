#include <stdint.h>
#include <stddef.h> // define NULL

#include "sps30_i2c.h"

#define SPS30_CHECK_STATUS(expr)     \
    do                               \
    {                                \
        Sps30Status retval = expr;   \
        if (retval != SPS30_SUCCESS) \
        {                            \
            return retval;           \
        }                            \
    } while (0)

static uint8_t sps30_calculate_crc8(uint8_t data[2]);

/**
 * @brief Reads the firmware version from the SPS30 device and verifies its checksum.
 *
 * @param[in] device The `Sps30Device` struct containing the I2C communication
 * functions.
 * @param[out] version A pointer to a `Sps30FirmwareVersion` struct where the
 * read firmware version is stored.
 *
 * @retval `SPS30_SUCCESS` Firmware version read and verified successfully.
 * @retval `SPS30_I2C_ERROR` I2C communication error.
 * @retval `SPS30_CRC_FAILURE` Firmware version checksum verification failed.
 */
int8_t sps30_read_firmware_version(Sps30Device *device, Sps30FirmwareVersion *version)
{
    if (device->i2c_write(SPS30_I2C_ADDRESS, (uint8_t[]){SPS30_I2C_CMD_READ_VERSION}, 2) != 0)
        return SPS30_I2C_ERROR;

    uint8_t rx_data[3];

    if (device->i2c_read(SPS30_I2C_ADDRESS, rx_data, 3) != 0)
        return SPS30_I2C_ERROR;

    version->major = rx_data[0];
    version->minor = rx_data[1];

    uint8_t received_checksum = rx_data[2];
    uint8_t calculated_checksum;

    if (device->calculate_crc != NULL)
    {
        if (device->calculate_crc(rx_data, 2, SPS30_CRC8_POLYNOMIAL, &calculated_checksum) != 0)
            return SPS30_CRC_FAILURE;
    }
    else
        calculated_checksum = sps30_calculate_crc8((uint8_t[]){rx_data[0], rx_data[1]});

    if (calculated_checksum != received_checksum)
        return SPS30_CRC_FAILURE;
    return SPS30_SUCCESS;
}

/**
 * @brief Calculates the CRC-8 checksum of two bytes of data.
 *
 * This function implements the CRC-8 algorithm as specified in the SPS30
 * datasheet. It takes two bytes of data and returns the calculated CRC-8
 * checksum.
 *
 * @param[in] data Two bytes of data to calculate the CRC-8 checksum for.
 *
 * @return The calculated CRC-8 checksum.
 */
static uint8_t sps30_calculate_crc8(uint8_t data[2])
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ SPS30_CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}