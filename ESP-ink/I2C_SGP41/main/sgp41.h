#ifndef INC_SGP41_H_
#define INC_SGP41_H_

#include "sensirion_gas_index_algorithm.h"

/* --- Constants --- */
#define SGP41_I2C_ADDRESS 0x59
#define SGP41_CRC8_POLYNOMIAL 0x31
#define SGP41_CMD_LEN 2
#define SGP41_DEF_TEMP 0x66, 0x66, 0x93
#define SGP41_DEF_RH 0x80, 0x00, 0xA2

/* --- Commands --- */
#define SGP41_CMD_EXEC_COND 0x26, 0x12
#define SGP41_CMD_MEAS_RAW 0x26, 0x19
#define SGP41_CMD_SELF_TEST 0x28, 0x0E
#define SGP41_CMD_HEATER_OFF 0x36, 0x15
#define SGP41_CMD_SERIAL_NO 0x36, 0x82

/* --- Command response lengths --- */
#define SGP41_RSP_LEN_COND 3
#define SGP41_RSP_LEN_MEAS_RAW 6
#define SGP41_RSP_LEN_SELF_TEST 3
#define SGP41_RSP_LEN_HEATER_OFF 0
#define SGP41_RSP_LEN_SERIAL_NO 9

/* --- Types --- */
typedef enum Sgp41Status
{
    SGP41_SUCCESS = 0,
    SGP41_I2C_ERROR = -1,
    SGP41_INVALID_VALUE = -2,
    SGP41_INVALID_OPERATION = -3,
    SGP41_POINTER_NULL = -4,
    SGP41_CRC_FAILURE = -5,
    SGP41_NOT_IMPLEMENTED = -6,
    SGP41_SELF_TEST_FAILURE = -7
} Sgp41Status;

typedef struct Sgp41RawData
{
    uint8_t sraw_voc[2]; // sraw_voc[0] = msb, sraw_voc[1] = lsb
    uint8_t crc_voc;
    uint8_t sraw_nox[2]; // sraw_nox[0] = msb, sraw_nox[1] = lsb
    uint8_t crc_nox;
} Sgp41RawData;

typedef struct Sgp41SerialNumber
{
    uint16_t serial_msb;
    uint16_t serial_mid;
    uint16_t serial_lsb;
    uint8_t msb_crc;
    uint8_t mid_crc;
    uint8_t lsb_crc;
} Sgp41SerialNumber;

typedef struct Sgp41Data
{
    int32_t voc_index;
    int32_t nox_index;
} Sgp41Data;

/* --- Function pointers --- */
// Target functions must return int8_t error code, 0 is the only accepted success value
typedef int8_t (*sgp41_i2c_write_t)(uint8_t address, const uint8_t *payload, uint8_t length);
typedef int8_t (*sgp41_i2c_read_t)(uint8_t address, uint8_t *payload, uint8_t length);
typedef int8_t (*sgp41_calculate_crc_t)(const uint8_t *data, uint8_t length, uint8_t polynomial, uint8_t *result);

typedef struct Sgp41Device
{
    sgp41_i2c_write_t i2c_write;
    sgp41_i2c_read_t i2c_read;
    sgp41_calculate_crc_t calculate_crc; // If NULL, internal CRC calculation will be used
    GasIndexAlgorithmParams gia_voc;     // Do not initialize, leave as NULL
    GasIndexAlgorithmParams gia_nox;     // Do not initialize, leave as NULL
} Sgp41Device;

/* --- Public functions --- */
Sgp41Status sgp41_initialize(Sgp41Device *device);
Sgp41Status sgp41_read_gas_indices(Sgp41Device *device, Sgp41Data *data);
Sgp41Status sgp41_read_raw_signals(Sgp41Device *device, Sgp41RawData *raw_data);

Sgp41Status sgp41_execute_conditioning(Sgp41Device *device);
Sgp41Status sgp41_measure_raw_signals(Sgp41Device *device, float *temp_celsius, float *rel_hum_pct);
Sgp41Status sgp41_get_serial_number(Sgp41Device *device, Sgp41SerialNumber *serial_number);
Sgp41Status sgp41_turn_heater_off(Sgp41Device *device);
Sgp41Status sgp41_execute_self_test(Sgp41Device *device);
Sgp41Status sgp41_evaluate_self_test(Sgp41Device *device);

Sgp41Status sgp41_check_crc(Sgp41Device *device, Sgp41RawData *raw_data);
Sgp41Status sgp41_convert_raw_data(Sgp41Device *device, Sgp41RawData *raw_data, Sgp41Data *data);

#endif /* INC_SGP41_H_ */