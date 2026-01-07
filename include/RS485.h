#ifndef RS485_H
#define RS485_H

#include "setup.h"


#define MAX_SAMPLES     10     // Upper bound to protect stack
#define FILTER_PCT      5.0f   // ±5%
#define NUM_REGISTERS   7


enum INTERROGATION_FRAME {
    ADDR,
    FUNC,
    STRT_ADDR_HI,
    STRT_ADDR_LO,
    DATA_LEN_HI,
    DATA_LEN_LO,
    CRC_HI,
    CRC_LO,
    FRAME_LEN
};


enum SOIL_REG {
    SOIL_MOISTURE,
    SOIL_TEMPERATURE,
    SOIL_EC,
    SOIL_PH,
    SOIL_N,
    SOIL_P,
    SOIL_K
};

enum RS485_STATUS : uint8_t { RS485_GOOD, RESPONSE_INCOMPLETE, CRC_MISMATCH, INVALID_PARAM };

RS485_STATUS read_Registers(HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint8_t sampleCount, uint16_t *results);
RS485_STATUS read_Registers_raw(HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint16_t numRegisters, uint16_t *results);
uint16_t calc_crc(uint8_t *data, uint8_t length);


#endif