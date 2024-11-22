#ifndef __SCD4X_I2C_H__
#define __SCD4X_I2C_H__

#include <stdint.h>

/* --- Constants --- */

#define SCD4X_I2C_ADDRESS 0x62
#define SCD4X_I2C_CMD_LENGTH 2
#define SCD4X_CRC8_POLYNOMIAL 0x31
#define SCD4X_CRC8_INITIAL_VALUE 0xFF
#define SCD4X_MEASUREMENT_LENGTH 9
#define SCD4X_TEMP_OFFSET_LENGTH 3

/* --- SCD4x commands --- */

#define SCD4X_CMD_START_PERIODIC_MEASUREMENT 0x21, 0xB1
#define SCD4X_CMD_READ_MEASUREMENT 0xEC, 0x05
#define SCD4X_CMD_STOP_PERIODIC_MEASUREMENT 0x3F, 0x86
#define SCD4X_CMD_SET_TEMPERATURE_OFFSET 0x24, 0x1D
#define SCD4X_CMD_GET_TEMPERATURE_OFFSET 0x23, 0x18
#define SCD4X_CMD_SET_SENSOR_ALTITUDE 0x24, 0x27
#define SCD4X_CMD_GET_SENSOR_ALTITUDE 0x23, 0x22
#define SCD4X_CMD_GET_SET_AMBIENT_PRESSURE 0xE0, 0x00
#define SCD4X_CMD_PERFORM_FORCED_RECALIBRATION 0x36, 0x2F
#define SCD4X_CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x24, 0x16
#define SCD4X_CMD_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x23, 0x13
#define SCD4X_CMD_SET_AUTOMATIC_SELF_CALIBRATION_TARGET 0x24, 0x3A
#define SCD4X_CMD_GET_AUTOMATIC_SELF_CALIBRATION_TARGET 0x23, 0x3F
#define SCD4X_CMD_START_LOW_POWER_PERIODIC_MEASUREMENT 0x21, 0xAC
#define SCD4X_CMD_GET_DATA_READY_STATUS 0xE4, 0xB8
#define SCD4X_CMD_PERSIST_SETTINGS 0x36, 0x15
#define SCD4X_CMD_GET_SERIAL_NUMBER 0x36, 0x82
#define SCD4X_CMD_PERFORM_SELF_TEST 0x36, 0x39
#define SCD4X_CMD_PERFORM_FACTORY_RESET 0x36, 0x32
#define SCD4X_CMD_REINIT 0x36, 0x46
#define SCD4X_CMD_GET_SENSOR_VARIANT 0x20, 0x2F

/* --- SCD41-only commands --- */

#define SCD4X_CMD_MEASURE_SINGLE_SHOT 0x21, 0x9D
#define SCD4X_CMD_MEASURE_SINGLE_SHOT_RHT_ONLY 0x21, 0x96
#define SCD4X_CMD_POWER_DOWN 0x36, 0xE0
#define SCD4X_CMD_WAKE_UP 0x36, 0xF6
#define SCD4X_CMD_SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD 0x24, 0x45
#define SCD4X_CMD_GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD 0x23, 0x40
#define SCD4X_CMD_SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD 0x24, 0x4E
#define SCD4X_CMD_GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD 0x23, 0x4B

/* --- Function pointers --- */
// Target functions must return int8_t error code, 0 is the only accepted success value

typedef int8_t (*scd4x_i2c_write_t)(uint8_t address, const uint8_t *payload, uint8_t length);
typedef int8_t (*scd4x_i2c_read_t)(uint8_t address, uint8_t *payload, uint8_t length);
typedef int8_t (*scd4x_delay_ms_t)(uint16_t ms);
typedef int8_t (*scd4x_calculate_crc_t)(const uint8_t *data, uint8_t length, uint8_t polynomial, uint8_t *result);

/* --- Types --- */

typedef enum Scd4xStatus
{
    SCD4X_SUCCESS = 0,
    SCD4X_I2C_ERROR = -1,
    SCD4X_INVALID_VALUE = -2,
    SCD4X_POINTER_NULL = -3,
    SCD4X_CRC_FAILURE = -4
} Scd4xStatus;

typedef struct Scd4xData
{
    uint16_t co2;               // ppm
    int16_t temperature;        // 100 * °C
    uint16_t relative_humidity; // 100 * %
} Scd4xData;

typedef struct Scd4xDevice
{
    scd4x_i2c_write_t i2c_write;
    scd4x_i2c_read_t i2c_read;
    scd4x_delay_ms_t delay_ms;
    scd4x_calculate_crc_t calculate_crc; // If NULL, internal SW CRC algorithm will be used
} Scd4xDevice;

/* --- SCD4x functions --- */

Scd4xStatus scd4x_start_periodic_measurement(Scd4xDevice *device);
Scd4xStatus scd4x_read_measurement(Scd4xDevice *device, Scd4xData *data);
Scd4xStatus scd4x_stop_periodic_measurement(Scd4xDevice *device);
Scd4xStatus scd4x_set_temperature_offset(Scd4xDevice *device, float offset);
Scd4xStatus scd4x_get_temperature_offset(Scd4xDevice *device, float *offset);
Scd4xStatus scd4x_set_sensor_altitude(Scd4xDevice *device, uint16_t altitude);
Scd4xStatus scd4x_get_sensor_altitude(Scd4xDevice *device, uint16_t *altitude);
Scd4xStatus scd4x_set_ambient_pressure(Scd4xDevice *device, uint16_t pressure);
Scd4xStatus scd4x_get_ambient_pressure(Scd4xDevice *device, uint16_t *pressure);
Scd4xStatus scd4x_perform_forced_recalibration(Scd4xDevice *device);
Scd4xStatus scd4x_set_automatic_self_calibration_enabled(Scd4xDevice *device, bool enabled);
Scd4xStatus scd4x_get_automatic_self_calibration_enabled(Scd4xDevice *device, bool *enabled);
Scd4xStatus scd4x_set_automatic_self_calibration_target(Scd4xDevice *device, uint8_t target);
Scd4xStatus scd4x_get_automatic_self_calibration_target(Scd4xDevice *device, uint8_t *target);
Scd4xStatus scd4x_set_automatic_self_calibration_interval(Scd4xDevice *device, uint8_t interval);
Scd4xStatus scd4x_start_low_power_periodic_measurement(Scd4xDevice *device);
Scd4xStatus scd4x_get_data_ready_status(Scd4xDevice *device, bool *ready);
Scd4xStatus scd4x_persist_settings(Scd4xDevice *device);
Scd4xStatus scd4x_get_serial_number(Scd4xDevice *device, uint16_t *serial_number);
Scd4xStatus scd4x_perform_self_test(Scd4xDevice *device);
Scd4xStatus scd4x_perform_factory_reset(Scd4xDevice *device);
Scd4xStatus scd4x_reinit(Scd4xDevice *device);
Scd4xStatus scd4x_get_sensor_variant(Scd4xDevice *device, uint8_t *variant);

/* --- SCD41-only functions --- */

Scd4xStatus scd41_measure_single_shot(Scd4xDevice *device, Scd4xData *data);
Scd4xStatus scd41_measure_single_shot_rht_only(Scd4xDevice *device, Scd4xData *data);
Scd4xStatus scd41_power_down(Scd4xDevice *device);
Scd4xStatus scd41_wake_up(Scd4xDevice *device);
Scd4xStatus scd41_set_automatic_self_calibration_initial_period(Scd4xDevice *device, uint8_t period);
Scd4xStatus scd41_get_automatic_self_calibration_initial_period(Scd4xDevice *device, uint8_t *period);
Scd4xStatus scd41_set_automatic_self_calibration_standard_period(Scd4xDevice *device, uint8_t period);
Scd4xStatus scd41_get_automatic_self_calibration_standard_period(Scd4xDevice *device, uint8_t *period);

#endif /* __SCD4X_I2C_H__ */