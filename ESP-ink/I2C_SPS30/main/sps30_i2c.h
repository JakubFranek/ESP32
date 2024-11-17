#ifndef __SPS30_I2C_H__
#define __SPS30_I2C_H__

#include <stdint.h>

/* --- Constants --- */

#define SPS30_I2C_ADDRESS 0x69
#define SPS30_CRC8_POLYNOMIAL 0x31 // x^8 + x^5 + x^4 + 1, initialization to 0xFF
#define SPS30_I2C_CMD_LENGTH 2     // bytes

/* --- Commands --- */

#define SPS30_I2C_CMD_START_MEASUREMENT 0x00, 0x10
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

/* --- Start Measurement Command Settings --- */

#define SPS30_I2C_FLOAT 0x03  // big-endian IEEE754 float
#define SPS30_I2C_UINT16 0x05 // big-endian unsigned 16-bit integer

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

typedef struct Sps30Device
{
    sps30_i2c_write_t i2c_write;
    sps30_i2c_read_t i2c_read;
    sps30_calculate_crc_t calculate_crc; // If NULL, internal SW CRC algorithm will be used
} Sps30Device;

/* --- Function Prototypes --- */

int8_t sps30_read_firmware_version(Sps30Device *device, Sps30FirmwareVersion *version);

#endif /* __SPS30_I2C_H__ */