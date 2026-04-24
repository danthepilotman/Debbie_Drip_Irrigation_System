#ifndef RS485_H
#define RS485_H

#include <HardwareSerial.h>  // hardware serial support
#include "setup.h"


#define MAX_SAMPLES     10     // sample limit to protect stack
#define FILTER_PCT      5.0f   // filter percentage
#define NUM_REGISTERS   7  // number of registers to read


enum INTERROGATION_FRAME {  // index names for frame fields
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


enum SOIL_REG {  // indices for sensor registers
    SOIL_MOISTURE,
    SOIL_TEMPERATURE,
    SOIL_EC,
    SOIL_PH,
    SOIL_N,
    SOIL_P,
    SOIL_K,
    SOIL_REG_SIZE
};

enum RS485_STATUS : uint8_t { RS485_GOOD, RESPONSE_INCOMPLETE, CRC_MISMATCH, INVALID_PARAM };  // RS485 status codes

extern const unsigned long SERIAL_BAUD_RATE;  // Set UI serial baud rate

extern HardwareSerial RS485Serial;  // Declare RS485Serial as external so other .cpp files can see it

void setup_RS485(); // Initialize RS485 hardware and serial settings
RS485_STATUS read_Registers(HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint8_t sampleCount, uint16_t *results);  // read registers (with sampling)
RS485_STATUS read_Registers_raw(HardwareSerial &serial, uint8_t deviceAddress, uint16_t startAddress, uint16_t numRegisters, uint16_t *results);  // raw register read
uint16_t calc_crc(uint8_t *data, uint8_t length);  // CRC calculation


#endif