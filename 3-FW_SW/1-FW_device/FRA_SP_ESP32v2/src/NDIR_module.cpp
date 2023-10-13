#include "NDIR_module.h"

bool NDIR_connection_check(uint8_t i2c_address){
    Wire.beginTransmission(i2c_address);
    if (Wire.endTransmission()==I2C_ERROR_OK){
        return false;
    }
    return true; //fail is true
}

bool readNDIRRegisters(uint8_t i2c_address, uint16_t *status_reg, uint16_t *gas_value, uint16_t *temperature) {
    Wire.beginTransmission(i2c_address);
    Wire.write(NDIR_REG_STATUS);  // Start from the first register
    Wire.endTransmission();

    Wire.requestFrom(i2c_address, (uint8_t)6);  // Request 6 bytes for 3 16-bit registers

    unsigned long startTime = millis();
    while (Wire.available() < 6 && (millis() - startTime) < NDIR_I2C_TIMEOUT_MS);

    if (Wire.available() < 6) {
        Serial.println("Timeout, device not responding.");
        return true;  // Fail is true
    }

    // Read the status register
    *status_reg = Wire.read() << 8;
    *status_reg |= Wire.read();

    // Read the gas value register
    *gas_value = Wire.read() << 8;
    *gas_value |= Wire.read();

    // Read the temperature register
    *temperature = Wire.read() << 8;
    *temperature |= Wire.read();

    return false;  // Everything is OK
}