/* --- Constants --- */
#define SGP41_I2C_ADDR 0x59
#define SGP41_CRC_POLY 0x31
#define SGP41_CMD_LEN 2
#define SGP41_DEF_TEMP {0x66, 0x66, 0x93}
#define SGP41_DEF_RH {0x80, 0x00, 0xA2}

/* --- Commands --- */
#define SGP41_CMD_COND {0x26, 0x12}
#define SGP41_CMD_MEAS_RAW {0x26, 0x19}
#define SGP41_CMD_SELF_TEST {0x28, 0x0E}
#define SGP41_CMD_HEATER_OFF {0x36, 0x15}
#define SGP41_CMD_SERIAL_NO {0x36, 0x82}

/* --- Command response lengths --- */
#define SGP41_RSP_LEN_COND 3
#define SGP41_RSP_LEN_MEAS_RAW 6
#define SGP41_RSP_LEN_SELF_TEST 3
#define SGP41_RSP_LEN_HEATER_OFF 0
#define SGP41_RSP_LEN_SERIAL_NO 9
