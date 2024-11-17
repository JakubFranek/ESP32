#include <stdint.h>
#include <stddef.h> // define NULL

#include "sgp41.h"

#define SGP41_CHECK_STATUS(expr)     \
    do                               \
    {                                \
        Sgp41Status retval = expr;   \
        if (retval != SGP41_SUCCESS) \
        {                            \
            return retval;           \
        }                            \
    } while (0)

static uint8_t sgp41_calculate_crc8(uint8_t data[2]);

/**
 * @brief Initializes an `Sgp41Device` by setting up the VOC and NOx gas index
 *        algorithm parameters.
 *
 * Call once before using any other SGP41 driver function.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure to be initialized.
 *
 * @retval `SGP41_SUCCESS` Initialization successful.
 */
Sgp41Status sgp41_initialize(Sgp41Device *device)
{
    GasIndexAlgorithm_init(&device->gia_voc, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
    GasIndexAlgorithm_init(&device->gia_nox, GasIndexAlgorithm_ALGORITHM_TYPE_NOX);
    return SGP41_SUCCESS;
}

/**
 * @brief Converts raw sensor data to gas index values using the VOC and NOx gas
 *        index algorithms.
 *
 * This function takes raw sensor data as input and converts it into gas index
 * values for VOC and NOx using the `GasIndexAlgorithm_process` function.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 * @param[in] raw_data Pointer to the raw sensor data structure.
 * @param[out] data Pointer to the `Sgp41Data` structure to store the converted data.
 *
 * @return `SGP41_SUCCESS` Convertion successful.
 */
Sgp41Status sgp41_convert_raw_data(Sgp41Device *device, Sgp41RawData *raw_data, Sgp41Data *data)
{
    int32_t sraw_voc_32bit = (raw_data->sraw_voc[0] << 8) + raw_data->sraw_voc[1];
    int32_t sraw_nox_32bit = (raw_data->sraw_nox[0] << 8) + raw_data->sraw_nox[1];

    GasIndexAlgorithm_process(&device->gia_voc, sraw_voc_32bit, &data->voc_index);
    GasIndexAlgorithm_process(&device->gia_nox, sraw_nox_32bit, &data->nox_index);
    return SGP41_SUCCESS;
}

/**
 * @brief Reads raw sensor data, verifies the CRC, and converts it to gas index
 *        values using the VOC and NOx gas index algorithms.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 * @param[out] data Pointer to the `Sgp41Data` structure to store the converted data.
 *
 * @retval `SGP41_SUCCESS` Command successful.
 * @retval `SGP41_I2C_ERROR` I2C error occured.
 * @retval `SGP41_CRC_FAILURE` CRC check failed.
 */
Sgp41Status sgp41_read_gas_indices(Sgp41Device *device, Sgp41Data *data)
{
    Sgp41RawData raw_data;

    SGP41_CHECK_STATUS(sgp41_read_raw_signals(device, &raw_data));
    SGP41_CHECK_STATUS(sgp41_check_crc(device, &raw_data));

    return sgp41_convert_raw_data(device, &raw_data, data);
}

/**
 * @brief Verifies the CRC of VOC and NOx raw sensor data.
 *
 * It uses a user-provided CRC calculation function if available, otherwise
 * it defaults to an internal CRC calculation.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 * @param[in] raw_data Pointer to the `Sgp41RawData` structure containing the raw data
 *                 and expected CRC values.
 *
 * @retval `SGP41_SUCCESS` Command successful.
 * @retval `SGP41_CRC_FAILURE` CRC check failed.
 */
Sgp41Status sgp41_check_crc(Sgp41Device *device, Sgp41RawData *raw_data)
{
    uint8_t crc_voc;
    uint8_t crc_nox;

    if (device->calculate_crc != NULL) // If CRC calculation function is provided by user
    {
        device->calculate_crc(raw_data->sraw_voc, 2, SGP41_CRC8_POLYNOMIAL, &crc_voc);
        device->calculate_crc(raw_data->sraw_nox, 2, SGP41_CRC8_POLYNOMIAL, &crc_nox);
    }
    else // If CRC calculation function is not provided by user
    {
        crc_voc = sgp41_calculate_crc8(raw_data->sraw_voc);
        crc_nox = sgp41_calculate_crc8(raw_data->sraw_nox);
    }

    if (crc_voc != raw_data->crc_voc || crc_nox != raw_data->crc_nox)
        return SGP41_CRC_FAILURE;

    return SGP41_SUCCESS;
}

/**
 * @brief Retrieves the serial number from the SGP41 sensor.
 *
 * This function reads the serial number from the SGP41 sensor and stores it in the
 * `Sgp41SerialNumber` structure. The serial number is composed of three 2-byte parts (MSB,
 * "mid", and LSB) and each part has a corresponding CRC value.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 * @param[out] serial_number Pointer to the `Sgp41SerialNumber` structure to store the
 *                      serial number.
 *
 * @retval `SGP41_SUCCESS` Command successful.
 * @retval `SGP41_I2C_ERROR` I2C error occured.
 * @retval `SGP41_CRC_FAILURE` CRC check failed.
 */
Sgp41Status sgp41_get_serial_number(Sgp41Device *device, Sgp41SerialNumber *serial_number)
{
    uint8_t rx_data[9];

    if (device->i2c_write(SGP41_I2C_ADDRESS, (uint8_t[]){SGP41_CMD_SERIAL_NO}, 2) != 0)
        return SGP41_I2C_ERROR;
    if (device->i2c_read(SGP41_I2C_ADDRESS, rx_data, 9) != 0)
        return SGP41_I2C_ERROR;

    serial_number->serial_msb = (rx_data[0] << 8) + rx_data[1];
    serial_number->msb_crc = rx_data[2];
    serial_number->serial_mid = (rx_data[3] << 8) + rx_data[4];
    serial_number->mid_crc = rx_data[5];
    serial_number->serial_lsb = (rx_data[6] << 8) + rx_data[7];
    serial_number->lsb_crc = rx_data[8];

    uint8_t crc_msb_check, crc_mid_check, crc_lsb_check;

    if (device->calculate_crc != NULL) // If CRC calculation function is provided by user
    {
        device->calculate_crc((uint8_t[]){rx_data[0], rx_data[1]}, 2, SGP41_CRC8_POLYNOMIAL, &crc_msb_check);
        device->calculate_crc((uint8_t[]){rx_data[3], rx_data[4]}, 2, SGP41_CRC8_POLYNOMIAL, &crc_mid_check);
        device->calculate_crc((uint8_t[]){rx_data[6], rx_data[7]}, 2, SGP41_CRC8_POLYNOMIAL, &crc_lsb_check);
    }
    else // If CRC calculation function is not provided by user
    {
        crc_msb_check = sgp41_calculate_crc8((uint8_t[]){rx_data[0], rx_data[1]});
        crc_mid_check = sgp41_calculate_crc8((uint8_t[]){rx_data[3], rx_data[4]});
        crc_lsb_check = sgp41_calculate_crc8((uint8_t[]){rx_data[6], rx_data[7]});
    }

    if (crc_msb_check != rx_data[2] || crc_mid_check != rx_data[5] || crc_lsb_check != rx_data[8])
        return SGP41_CRC_FAILURE;
    return SGP41_SUCCESS;
}

/**
 * @brief Executes the conditioning command on the SGP41 sensor.
 *
 * WARNING: This function must be followed up by a measurement command or heater off command
 * within 10 seconds to prevent damage to the sensor.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 *
 * @retval `SGP41_SUCCESS` Command successful.
 * @retval `SGP41_I2C_ERROR` I2C error occured.
 */
Sgp41Status sgp41_execute_conditioning(Sgp41Device *device)
{
    uint8_t tx_data[8] = {SGP41_CMD_EXEC_COND, SGP41_DEF_RH, SGP41_DEF_TEMP};

    if (device->i2c_write(SGP41_I2C_ADDRESS, tx_data, 8) != 0)
        return SGP41_I2C_ERROR;
    return SGP41_SUCCESS;
}

/**
 * @brief Writes the measurement command and optional temperature and humidity
 *        values to the SGP41 sensor.
 *
 * To read the raw sensor data, wait for 50 ms and call `sgp41_read_raw_signals`.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 * @param[in] temp_celsius Optional pointer to the temperature value in Celsius degrees.
 * @param[in] rel_hum_pct Optional pointer to the relative humidity value in percent.
 *
 * @retval `SGP41_SUCCESS` Command successful.
 * @retval `SGP41_I2C_ERROR` I2C error occured.
 */
Sgp41Status sgp41_measure_raw_signals(Sgp41Device *device, float *temp_celsius, float *rel_hum_pct)
{
    uint8_t tx_data[8] = {SGP41_CMD_MEAS_RAW, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t rel_hum_data[3] = {SGP41_DEF_RH};
    uint8_t temp_data[3] = {SGP41_DEF_TEMP};

    uint16_t temp_ticks;
    uint16_t rel_hum_ticks;

    if (temp_celsius != NULL)
    {
        temp_ticks = (uint16_t)((*temp_celsius + 45) * 65535 / 175);
        temp_data[0] = (uint8_t)((temp_ticks >> 8) & 0xFF);
        temp_data[1] = (uint8_t)(temp_ticks & 0xFF);
        temp_data[2] = sgp41_calculate_crc8((uint8_t[]){temp_data[0], temp_data[1]});
    }

    if (rel_hum_pct != NULL)
    {
        rel_hum_ticks = (uint16_t)(*rel_hum_pct * 65535 / 100);
        rel_hum_data[0] = (uint8_t)((rel_hum_ticks >> 8) & 0xFF);
        rel_hum_data[1] = (uint8_t)(rel_hum_ticks & 0xFF);
        rel_hum_data[2] = sgp41_calculate_crc8((uint8_t[]){rel_hum_data[0], rel_hum_data[1]});
    }

    tx_data[2] = rel_hum_data[0];
    tx_data[3] = rel_hum_data[1];
    tx_data[4] = rel_hum_data[2];
    tx_data[5] = temp_data[0];
    tx_data[6] = temp_data[1];
    tx_data[7] = temp_data[2];

    if (device->i2c_write(SGP41_I2C_ADDRESS, tx_data, 8) != 0)
        return SGP41_I2C_ERROR;
    return SGP41_SUCCESS;
}

/**
 * @brief Reads raw sensor data from the SGP41 sensor.
 *
 * This function reads the raw sensor data from the SGP41 sensor and stores it in
 * the `Sgp41RawData` structure. The raw data contains the raw VOC and NOx sensor
 * values, as well as the expected CRC values for the raw data.
 *
 * Should be preceded by a call to `sgp41_measure_raw_signals` (with a minimum interval of 50 ms).
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 * @param[out] raw_data Pointer to the `Sgp41RawData` structure to store the raw data.
 *
 * @retval `SGP41_SUCCESS` Read successful.
 * @retval `SGP41_I2C_ERROR` I2C error occured.
 */
Sgp41Status sgp41_read_raw_signals(Sgp41Device *device, Sgp41RawData *raw_data)
{
    uint8_t rx_data[6];

    if (device->i2c_read(SGP41_I2C_ADDRESS, rx_data, 6) != 0)
        return SGP41_I2C_ERROR;

    raw_data->sraw_voc[0] = rx_data[0];
    raw_data->sraw_voc[1] = rx_data[1];
    raw_data->crc_voc = rx_data[2];
    raw_data->sraw_nox[0] = rx_data[3];
    raw_data->sraw_nox[1] = rx_data[4];
    raw_data->crc_nox = rx_data[5];

    return SGP41_SUCCESS;
}

/**
 * @brief Turns the SGP41 sensor's heater off.
 *
 * This function sends the command to turn the heater off to the SGP41 sensor.
 * The sensor enters idle mode.
 *
 * @param[in] device Pointer to the `Sgp41Device` structure.
 *
 * @retval `SGP41_SUCCESS` Command successful.
 * @retval `SGP41_I2C_ERROR` I2C error occured.
 */
Sgp41Status sgp41_turn_heater_off(Sgp41Device *device)
{
    if (device->i2c_write(SGP41_I2C_ADDRESS, (uint8_t[]){SGP41_CMD_HEATER_OFF}, 8) != 0)
        return SGP41_I2C_ERROR;
    return SGP41_SUCCESS;
}

/**
 * @brief Executes the self-test command on the SGP41 sensor.
 *
 * This function sends the self-test command to the SGP41 sensor. The result
 * of the self-test can be evaluated using the sgp41_evaluate_self_test
 * function, after a minimum interval of 320 ms.
 *
 * @param[in] device Pointer to the Sgp41Device structure.
 *
 * @retval `SGP41_SUCCESS` Command successful.
 * @retval `SGP41_I2C_ERROR` I2C error occured.
 */
Sgp41Status sgp41_execute_self_test(Sgp41Device *device)
{
    if (device->i2c_write(SGP41_I2C_ADDRESS, (uint8_t[]){SGP41_CMD_SELF_TEST}, 2) != 0)
        return SGP41_I2C_ERROR;
    return SGP41_SUCCESS;
}

/**
 * @brief Evaluate the result of the SGP41 sensor's self-test.
 *
 * Reads the self-test result from the SGP41 sensor and verifies the integrity of
 * the data using CRC.
 *
 * Call this function >320 ms after a call to `sgp41_execute_self_test`.
 *
 * @param[in] device Pointer to an `Sgp41Device` structure representing the sensor instance.
 *
 * @retval `SGP41_SUCCESS` Self-test completed successfully.
 * @retval `SGP41_I2C_ERROR` I2C communication error occurred.
 * @retval `SGP41_CRC_FAILURE` CRC verification of the received data failed.
 * @retval `SGP41_SELF_TEST_FAILURE` The self-test indicated a sensor failure.
 */
Sgp41Status sgp41_evaluate_self_test(Sgp41Device *device)
{
    uint8_t rx_data[3];
    uint8_t crc_check;

    if (device->i2c_read(SGP41_I2C_ADDRESS, rx_data, 3) != 0)
        return SGP41_I2C_ERROR;

    if (device->calculate_crc != NULL)
        device->calculate_crc((uint8_t[]){rx_data[0], rx_data[1]}, 2, SGP41_CRC8_POLYNOMIAL, &crc_check);
    else
        crc_check = sgp41_calculate_crc8((uint8_t[]){rx_data[0], rx_data[1]});

    if (crc_check != rx_data[2])
        return SGP41_CRC_FAILURE;

    if ((rx_data[1] & 0x03) != 0)
        return SGP41_SELF_TEST_FAILURE;

    return SGP41_SUCCESS;
}

/**
 * @brief Calculates the CRC-8 checksum of two bytes of data.
 *
 * This function implements the CRC-8 algorithm as specified in the SGP41
 * datasheet. It takes two bytes of data and returns the calculated CRC-8
 * checksum.
 *
 * @param[in] data Two bytes of data to calculate the CRC-8 checksum for.
 *
 * @return The calculated CRC-8 checksum.
 */
static uint8_t sgp41_calculate_crc8(uint8_t data[2])
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ SGP41_CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}