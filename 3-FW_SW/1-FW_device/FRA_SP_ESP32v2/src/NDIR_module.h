#ifndef NDIR_MODULE_H
#define NDIR_MODULE_H

#include "Arduino.h"
#include <Wire.h>

#define NDIR_ADDR 0x50

#define NDIR_REG_STATUS     0x00
#define NDIR_REG_GAS_value  0x01
#define NDIR_REG_TH         0x02 //temperature value
#define NDIR_REG_GAS_REF    0x03 //reference value
#define NDIR_REG_GAS_ACT    0x04 //detector value
#define NDIR_REG_RES        0x05 //reserved

#define NDIR_I2C_TIMEOUT_MS 10

bool NDIR_connection_check(uint8_t i2c_address);
bool readNDIRRegisters(uint8_t i2c_address, uint16_t *status_reg, uint16_t *gas_value, uint16_t *temperature);


#endif