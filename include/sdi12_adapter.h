#ifndef SDI12_ADAPTER_H
#define SDI12_ADAPTER_H

#include <Wire.h>
#include <inttypes.h>
#include "const.h"

/******************************************************************************
 * Class for communicating with the arduino nano I2C slave that acts as an
 * SDI12 adapter because there is no SDI12 library for ESP32.
 * 
 *****************************************************************************/
class SDI12Adapter
{
public:
	SDI12Adapter(uint8_t i2c_addr);
	bool begin(uint8_t sda_pin, uint8_t scl_pin);
	int send(char *cmd);
	int read();
	bool check_crc();
	const char *get_data();
private:
	/** Received data buffer */
	char _buff[SDI12_ADAPTER_BUFF_SIZE] = "";
	/** I2C address of slave */
	uint8_t _i2c_addr = 0;
	/** I2c bus */
	TwoWire _i2cbus = TwoWire(SDI12_ADAPTER_I2C_BUS);

};

#endif