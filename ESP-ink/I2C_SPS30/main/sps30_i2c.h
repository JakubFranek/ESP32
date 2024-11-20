#ifndef __SPS30_I2C_H__
#define __SPS30_I2C_H__

#include <stdint.h>
#include <stdbool.h>

/* --- Constants --- */

#define SPS30_I2C_ADDRESS 0x69
#define SPS30_I2C_CMD_LENGTH 2                   // bytes
#define SPS30_I2C_CMD_START_MEASUREMENT_LENGTH 5 // bytes
#define SPS30_I2C_PRODUCT_TYPE_LENGTH 12         // bytes
#define SPS30_I2C_SERIAL_NUMBER_LENGTH 48        // bytes
#define SPS30_I2C_FIRMWARE_LENGTH 3              // bytes
#define SPS30_I2C_STATUS_FLAGS_LENGTH 6          // bytes
#define SPS30_I2C_FLOAT_DATA_LENGTH 60           // bytes
#define SPS30_I2C_UINT16_DATA_LENGTH 30          // bytes
#define SPS30_CRC8_POLYNOMIAL 0x31               // x^8 + x^5 + x^4 + 1, initialization to 0xFF

/* --- Start Measurement Command Settings --- */

#define SPS30_I2C_FLOAT 0x03, 0x00, 0xAC  // big-endian IEEE754 float
#define SPS30_I2C_UINT16 0x05, 0x00, 0xF6 // big-endian unsigned 16-bit integer

/* --- Commands --- */

#define SPS30_I2C_CMD_START_MEASUREMENT 0x00, 0x10
#define SPS30_I2C_CMD_START_MEASUREMENT_FLOAT SPS30_I2C_CMD_START_MEASUREMENT, SPS30_I2C_FLOAT
#define SPS30_I2C_CMD_START_MEASUREMENT_UINT16 SPS30_I2C_CMD_START_MEASUREMENT, SPS30_I2C_UINT16
#define SPS30_I2C_CMD_STOP_MEASUREMENT 0x01, 0x04
#define SPS30_I2C_CMD_READ_DATA_READY_FLAG 0x02, 0x02
#define SPS30_I2C_CMD_READ_MEASURED_VALUES 0x03, 0x00
#define SPS30_I2C_CMD_SLEEP 0x10, 0x01  // version >= 2.0
#define SPS30_I2C_CMD_WAKEUP 0x11, 0x03 // version >= 2.0
#define SPS30_I2C_CMD_START_FAN_CLEANING 0x56, 0x07
#define SPS30_I2C_CMD_READ_WRITE_AUTO_CLEANING_INTERVAL 0x80, 0x04
#define SPS30_I2C_CMD_READ_PRODUCT_TYPE 0xD0, 0x02
#define SPS30_I2C_CMD_READ_SERIAL_NUMBER 0xD0, 0x33
#define SPS30_I2C_CMD_READ_VERSION 0xD1, 0x00
#define SPS30_I2C_CMD_READ_DEVICE_STATUS_REGISTER 0xD2, 0x06  // version >= 2.2
#define SPS30_I2C_CMD_CLEAR_DEVICE_STATUS_REGISTER 0xD2, 0x10 // version >= 2.0
#define SPS30_I2C_CMD_RESET 0xD3, 0x04

/* --- Function pointers --- */
// Target functions must return int8_t error code, 0 is the only accepted success value

typedef int8_t (*sps30_i2c_write_t)(uint8_t address, const uint8_t *payload, uint8_t length);
typedef int8_t (*sps30_i2c_read_t)(uint8_t address, uint8_t *payload, uint8_t length);
typedef int8_t (*sps30_calculate_crc_t)(const uint8_t *data, uint8_t length, uint8_t polynomial, uint8_t *result);

/* --- Types --- */

typedef enum Sps30Status
{
    SPS30_SUCCESS = 0,
    SPS30_I2C_ERROR = -1,
    SPS30_INVALID_VALUE = -2,
    SPS30_POINTER_NULL = -3,
    SPS30_CRC_FAILURE = -4
} Sps30Status;

typedef struct Sps30FirmwareVersion
{
    uint8_t major;
    uint8_t minor;
} Sps30FirmwareVersion;

typedef enum Sps30DataFormat
{
    SPS30_FLOAT = 0,
    SPS30_UINT16 = -1
} Sps30DataFormat;

typedef struct Sps30StatusFlags
{
    bool speed_warning;
    bool laser_error;
    bool fan_error;
} Sps30StatusFlags;

/**
 * @brief A structure containing the `float` data read from the SPS30 sensor.
 *
 * Mass concentration unit is `ug/m^3`, number concentration unit is `#/cm^3`
 * and typical particle size unit is `nm`.
 */
typedef struct Sps30FloatData
{
    float mass_concentration_pm1_0;
    float mass_concentration_pm2_5;
    float mass_concentration_pm4_0;
    float mass_concentration_pm10_0;
    float number_concentration_pm0_5;
    float number_concentration_pm1_0;
    float number_concentration_pm2_5;
    float number_concentration_pm4_0;
    float number_concentration_pm10_0;
    float typical_particle_size;
} Sps30FloatData;

/**
 * @brief A structure containing the `uint16_t` data read from the SPS30 sensor.
 *
 * Mass concentration unit is `ug/m^3`, number concentration unit is `#/cm^3`
 * and typical particle size unit is `nm`.
 */
typedef struct Sps30Uint16Data
{
    uint16_t mass_concentration_pm1_0;
    uint16_t mass_concentration_pm2_5;
    uint16_t mass_concentration_pm4_0;
    uint16_t mass_concentration_pm10_0;
    uint16_t number_concentration_pm0_5;
    uint16_t number_concentration_pm1_0;
    uint16_t number_concentration_pm2_5;
    uint16_t number_concentration_pm4_0;
    uint16_t number_concentration_pm10_0;
    uint16_t typical_particle_size;
} Sps30Uint16Data;

typedef struct Sps30Device
{
    sps30_i2c_write_t i2c_write;
    sps30_i2c_read_t i2c_read;
    sps30_calculate_crc_t calculate_crc; // If NULL, internal SW CRC algorithm will be used
} Sps30Device;

/* --- Function Prototypes --- */

int8_t sps30_start_measurement(Sps30Device *device, Sps30DataFormat data_format);
int8_t sps30_stop_measurement(Sps30Device *device);
int8_t sps30_read_data_ready_flag(Sps30Device *device, bool *data_ready);
int8_t sps30_read_measured_values_float(Sps30Device *device, Sps30FloatData *data); // TODO: overload for uint16
int8_t sps30_read_measured_values_uint16(Sps30Device *device, Sps30Uint16Data *data);
int8_t sps30_sleep(Sps30Device *device);
int8_t sps30_wake_up(Sps30Device *device);
int8_t sps30_start_fan_cleaning(Sps30Device *device);
int8_t sps30_read_auto_cleaning_interval(Sps30Device *device, uint32_t *interval);
int8_t sps30_set_auto_cleaning_interval(Sps30Device *device, uint32_t interval);
int8_t sps30_read_product_type(Sps30Device *device, char *product_type);
int8_t sps30_read_serial_number(Sps30Device *device, char *serial_number);
int8_t sps30_read_firmware_version(Sps30Device *device, Sps30FirmwareVersion *version);
int8_t sps30_read_device_status_flags(Sps30Device *device, Sps30StatusFlags *status_flags);
int8_t sps30_clear_device_status_flags(Sps30Device *device);
int8_t sps30_reset(Sps30Device *device);

#endif /* __SPS30_I2C_H__ */