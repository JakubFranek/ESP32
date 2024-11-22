#include <stdint.h>  // define uint8_t, uint16_t etc.
#include <stddef.h>  // define NULL
#include <string.h>  // define memcpy
#include <stdbool.h> // define bool

#include "scd4x.h"

/**
 * Error-checking macro: if `expr` is not `SCD4X_SUCCESS`, this macro returns `expr`,
 * exiting the function where this macro was used immediately.
 */
#define SCD4X_CHECK_STATUS(expr)     \
    do                               \
    {                                \
        Scd4xStatus retval = expr;   \
        if (retval != SCD4X_SUCCESS) \
        {                            \
            return retval;           \
        }                            \
    } while (0)

/**
 * Error-checking macro: if `expr` is `NULL`, this macro returns `SCD4X_POINTER_NULL`,
 * exiting the function where this macro was used immediately.
 */
#define SCD4X_CHECK_NULL(expr)         \
    do                                 \
    {                                  \
        if (expr == NULL)              \
        {                              \
            return SCD4X_POINTER_NULL; \
        }                              \
    } while (0)

/* --- Private function prototypes --- */

static Scd4xStatus scd4x_check_checksum(Scd4xDevice *device, uint8_t data[2], uint8_t checksum);
static Scd4xStatus scd4x_calculate_checksum(Scd4xDevice *device, uint8_t data[2], uint8_t *checksum);
static uint8_t scd4x_calculate_crc8(uint8_t data[2]);
static Scd4xStatus scd4x_check_device(Scd4xDevice *device);

/* --- Function definitions --- */

/**
 * @brief Starts the periodic measurement mode.
 *
 * In this mode, the sensor will continuously take measurements with a 5 second period.
 * The data ready status can be checked using the `get_data_ready_status` function.
 * The measurement data can be read using the `request_measurement` and `read_measurement` functions.
 * The periodic measurement mode can be stopped using the `stop_periodic_measurement` function.
 *
 * @param[in] device The `Scd4xDevice` struct containing the I2C functions.
 *
 * @retval `SCD4X_SUCCESS` The command was sent to the device.
 * @retval `SCD4X_I2C_ERROR` An I2C communication error occurred.
 * @retval `SCD4X_POINTER_NULL` The `device` pointer is `NULL`.
 */
Scd4xStatus scd4x_scd4x_start_periodic_measurement(Scd4xDevice *device)
{
    SCD4X_CHECK_STATUS(scd4x_check_device(device));

    if (device->i2c_write(SCD4X_I2C_ADDRESS, (uint8_t[]){SCD4X_CMD_START_PERIODIC_MEASUREMENT}, SCD4X_I2C_CMD_LENGTH) != 0)
        return SCD4X_I2C_ERROR;
    return SCD4X_SUCCESS;
}

/**
 * @brief Reads a measurement from the sensor.
 *
 * @param[in] device The `Scd4xDevice` struct containing the I2C functions.
 *
 * @retval `SCD4X_SUCCESS` The command was sent to the device.
 * @retval `SCD4X_I2C_ERROR` An I2C communication error occurred.
 * @retval `SCD4X_POINTER_NULL` The `device` pointer is `NULL`.
 * @retval `SCD4X_CRC_FAILURE` CRC verification failed.
 */
Scd4xStatus scd4x_read_measurement(Scd4xDevice *device, Scd4xData *data)
{
    SCD4X_CHECK_STATUS(scd4x_check_device(device));

    if (device->i2c_write(SCD4X_I2C_ADDRESS, (uint8_t[]){SCD4X_CMD_READ_MEASUREMENT}, SCD4X_I2C_CMD_LENGTH) != 0)
        return SCD4X_I2C_ERROR;

    device->delay_ms(1);

    uint8_t data_buf[SCD4X_MEASUREMENT_LENGTH];
    if (device->i2c_read(SCD4X_I2C_ADDRESS, data_buf, SCD4X_MEASUREMENT_LENGTH) != 0)
        return SCD4X_I2C_ERROR;

    for (int i = 0; i < SCD4X_MEASUREMENT_LENGTH; i = i + 3)
        SCD4X_CHECK_STATUS(scd4x_check_checksum(device, (uint8_t[]){data_buf[i], data_buf[i + 1]}, data_buf[i + 2]));

    data->co2 = data_buf[0] << 8 | data_buf[1];
    data->temperature = ((4375 * (int32_t)(data_buf[3] << 8 | data_buf[4])) >> 14) - 4500;
    data->relative_humidity = ((2500 * (uint32_t)(data_buf[6] << 8 | data_buf[7])) >> 14);

    return SCD4X_SUCCESS;
}

/**
 * @brief Stops the periodic measurement mode.
 *
 * WARNING: Do not issue any commands after calling this function for 500 milliseconds.
 *
 * @param[in] device The `Scd4xDevice` struct containing the I2C functions.
 *
 * @retval `SCD4X_SUCCESS` The command was sent to the device.
 * @retval `SCD4X_I2C_ERROR` An I2C communication error occurred.
 * @retval `SCD4X_POINTER_NULL` The `device` pointer is `NULL`.
 */
Scd4xStatus scd4x_stop_periodic_measurement(Scd4xDevice *device)
{
    SCD4X_CHECK_STATUS(scd4x_check_device(device));

    if (device->i2c_write(SCD4X_I2C_ADDRESS, (uint8_t[]){SCD4X_CMD_STOP_PERIODIC_MEASUREMENT}, SCD4X_I2C_CMD_LENGTH) != 0)
        return SCD4X_I2C_ERROR;
    return SCD4X_SUCCESS;
}

/**
 * @brief Sets the temperature offset.
 *
 * The temperature offset is added to the internal temperature measurement.
 * Recommended values are between 0 and 20 degrees Celsius. Default value is 4 degrees Celsius.
 * To save the setting to the non-volatile EEPROM, call `scd4x_persist_settings`.
 *
 * WARNING: Do not issue any commands after calling this function for 1 millisecond.
 *
 * @param[in] device The `Scd4xDevice` struct containing the I2C functions.
 * @param[in] offset The temperature offset to set, in degrees Celsius, of type `float`.
 *
 * @retval `SCD4X_SUCCESS` The offset was set successfully.
 * @retval `SCD4X_I2C_ERROR` An I2C communication error occurred.
 * @retval `SCD4X_POINTER_NULL` The `device` pointer is `NULL`.
 * @retval `SCD4X_CRC_FAILURE` CRC calculation failed.
 */
Scd4xStatus scd4x_set_temperature_offset(Scd4xDevice *device, float offset)
{
    SCD4X_CHECK_STATUS(scd4x_check_device(device));

    int16_t offset_int = offset * 374; // (2**16 - 1) / 175 = 374.4857
    uint8_t tx_data[SCD4X_TEMP_OFFSET_LENGTH];
    tx_data[0] = offset_int >> 8;
    tx_data[1] = offset_int & 0xFF;
    tx_data[2] = scd4x_calculate_crc8(tx_data);

    if (device->i2c_write(SCD4X_I2C_ADDRESS, (uint8_t[]){SCD4X_CMD_SET_TEMPERATURE_OFFSET, tx_data[0], tx_data[1], tx_data[2]},
                          SCD4X_I2C_CMD_LENGTH + SCD4X_TEMP_OFFSET_LENGTH) != 0)
        return SCD4X_I2C_ERROR;
    return SCD4X_SUCCESS;
}

/**
 * @brief Gets the current temperature offset.
 *
 * The temperature offset is added to the internal temperature measurement.
 * The default value is 4 degrees Celsius.
 *
 * @param[in] device The `Scd4xDevice` struct containing the I2C functions.
 * @param[out] offset The current temperature offset, in degrees Celsius, of type `float`.
 *
 * @retval `SCD4X_SUCCESS` The offset was read successfully.
 * @retval `SCD4X_I2C_ERROR` An I2C communication error occurred.
 * @retval `SCD4X_POINTER_NULL` The `device` pointer is `NULL`.
 * @retval `SCD4X_CRC_FAILURE` CRC calculation failed.
 */
Scd4xStatus scd4x_get_temperature_offset(Scd4xDevice *device, float *offset)
{
    SCD4X_CHECK_STATUS(scd4x_check_device(device));

    if (device->i2c_write(SCD4X_I2C_ADDRESS, (uint8_t[]){SCD4X_CMD_GET_TEMPERATURE_OFFSET}, SCD4X_I2C_CMD_LENGTH) != 0)
        return SCD4X_I2C_ERROR;

    device->delay_ms(1);

    uint8_t rx_data[SCD4X_TEMP_OFFSET_LENGTH];
    if (device->i2c_read(SCD4X_I2C_ADDRESS, rx_data, SCD4X_TEMP_OFFSET_LENGTH) != 0)
        return SCD4X_I2C_ERROR;

    SCD4X_CHECK_STATUS(scd4x_check_checksum(device, rx_data, rx_data[2]));

    int16_t offset_int = rx_data[0] << 8 | rx_data[1];
    *offset = offset_int / 374.4857; // (2**16 - 1) / 175 = 374.4857

    return SCD4X_SUCCESS;
}

Scd4xStatus scd4x_set_sensor_altitude(Scd4xDevice *device, uint16_t altitude) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_get_sensor_altitude(Scd4xDevice *device, uint16_t *altitude) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_set_ambient_pressure(Scd4xDevice *device, uint16_t pressure) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_get_ambient_pressure(Scd4xDevice *device, uint16_t *pressure) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_perform_forced_recalibration(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_set_automatic_self_calibration_enabled(Scd4xDevice *device, bool enabled) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_get_automatic_self_calibration_enabled(Scd4xDevice *device, bool *enabled) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_set_automatic_self_calibration_target(Scd4xDevice *device, uint8_t target) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_get_automatic_self_calibration_target(Scd4xDevice *device, uint8_t *target) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_set_automatic_self_calibration_interval(Scd4xDevice *device, uint8_t interval) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_start_low_power_periodic_measurement(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_get_data_ready_status(Scd4xDevice *device, bool *ready) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_persist_settings(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_get_serial_number(Scd4xDevice *device, uint16_t *serial_number) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_perform_self_test(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_perform_factory_reset(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_reinit(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd4x_get_sensor_variant(Scd4xDevice *device, uint8_t *variant) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_measure_single_shot(Scd4xDevice *device, Scd4xData *data) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_measure_single_shot_rht_only(Scd4xDevice *device, Scd4xData *data) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_power_down(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_wake_up(Scd4xDevice *device) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_set_automatic_self_calibration_initial_period(Scd4xDevice *device, uint8_t period) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_get_automatic_self_calibration_initial_period(Scd4xDevice *device, uint8_t *period) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_set_automatic_self_calibration_standard_period(Scd4xDevice *device, uint8_t period) { return SCD4X_SUCCESS; }
Scd4xStatus scd41_get_automatic_self_calibration_standard_period(Scd4xDevice *device, uint8_t *period) { return SCD4X_SUCCESS; }

/**
 * @brief Verifies the checksum for a given data array using the SCD4x device's CRC function.
 *
 * This function calculates the CRC-8 checksum of the provided 2-byte data array and compares it
 * with the given checksum. If the SCD4x device has a custom CRC calculation function defined, it uses that function.
 * Otherwise, it falls back to the default software CRC-8 algorithm.
 *
 * @param[in] device The `Scd4xDevice` struct containing the CRC calculation function.
 * @param[in] data A 2-byte array for which the checksum is to be verified.
 * @param[in] checksum A `uint8_t` containing the expected checksum.
 *
 * @retval `SCD4X_SUCCESS` Checksum verified successfully.
 * @retval `SCD4X_CRC_FAILURE` CRC verification using the device's function failed.
 */
static Scd4xStatus scd4x_check_checksum(Scd4xDevice *device, uint8_t data[2], uint8_t checksum)
{
    uint8_t calculated_checksum = 0;
    SCD4X_CHECK_STATUS(scd4x_calculate_checksum(device, data, &calculated_checksum));

    if (calculated_checksum != checksum)
        return SCD4X_CRC_FAILURE;

    return SCD4X_SUCCESS;
}

/**
 * @brief Calculates the checksum for a given data array using the SCD4x device's CRC function.
 *
 * This function calculates the CRC-8 checksum of the provided 2-byte data array.
 * If the SCD4x device has a custom CRC calculation function defined, it uses that function.
 * Otherwise, it falls back to the default software CRC-8 algorithm.
 *
 * @param[in] device The `Scd4xDevice` struct containing the CRC calculation function.
 * @param[in] data A 2-byte array for which the checksum is to be calculated.
 * @param[out] checksum A pointer to a `uint8_t` where the calculated checksum is stored.
 *
 * @retval `SCD4X_SUCCESS` Checksum calculated successfully.
 * @retval `SCD4X_CRC_FAILURE` CRC calculation using the device's function failed.
 */
static Scd4xStatus scd4x_calculate_checksum(Scd4xDevice *device, uint8_t data[2], uint8_t *checksum)
{
    if (device->calculate_crc != NULL)
    {
        if (device->calculate_crc(data, 2, SCD4X_CRC8_POLYNOMIAL, checksum) != 0)
            return SCD4X_CRC_FAILURE;
    }
    else
        *checksum = scd4x_calculate_crc8(data);

    return SCD4X_SUCCESS;
}

/**
 * @brief Calculates the CRC-8 checksum of two bytes of data.
 *
 * This function implements the CRC-8 algorithm as specified in the SCD4x
 * datasheet. It takes two bytes of data and returns the calculated CRC-8
 * checksum.
 *
 * @param[in] data Two bytes of data to calculate the CRC-8 checksum for.
 *
 * @return The calculated CRC-8 checksum.
 */
static uint8_t scd4x_calculate_crc8(uint8_t data[2])
{
    uint8_t crc = SCD4X_CRC8_INITIAL_VALUE;
    for (int i = 0; i < 2; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ SCD4X_CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

/**
 * @brief Checks whether the provided `Scd4xDevice` struct contains valid pointers.
 *
 * Returns -1 if the `device`pointer is NULL, or if the `i2c_write`, `i2c_read` or
 * `delay_ms` function pointers are `NULL`.
 *
 * WARNING: This function does not check whether the function pointers are
 * pointing to valid functions.
 *
 * @param[in] device The `Scd4xDevice` struct to be checked.
 *
 * @return 0 if the struct pointers are valid, -1 otherwise.
 */
static Scd4xStatus scd4x_check_device(Scd4xDevice *device)
{
    if (device == NULL)
        return SCD4X_POINTER_NULL;
    if (device->i2c_write == NULL || device->i2c_read == NULL || device->delay_ms == NULL)
        return SCD4X_POINTER_NULL;
    return SCD4X_SUCCESS;
}