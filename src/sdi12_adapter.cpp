#include "sdi12_adapter.h"
#include <HardwareSerial.h>
#include <Wire.h>


/******************************************************************************
 * Constructor
 * @param i2c_addr I2C address of adapter
 *****************************************************************************/
SDI12Adapter::SDI12Adapter(uint8_t i2c_addr)
{
	_i2c_addr = i2c_addr;
}

/******************************************************************************
 * Initialize Wire
 * @return False on error
 *****************************************************************************/
bool SDI12Adapter::begin(uint8_t sda_pin, uint8_t scl_pin)
{
	pinMode(PIN_SDI12_I2C1_SDA, INPUT_PULLUP);
	pinMode(PIN_SDI12_I2C1_SCL, INPUT_PULLUP);

	return _i2cbus.begin(PIN_SDI12_I2C1_SDA, PIN_SDI12_I2C1_SCL);
}

/******************************************************************************
 * Send SDI12 command
 * The command is immediately sent to the SDI12 device
 * @param cmd Command to send
 *****************************************************************************/
int SDI12Adapter::send(char *cmd)
{
	Serial.print(F("SDI12 send: "));
	Serial.println(cmd);
	_i2cbus.beginTransmission(_i2c_addr);
	_i2cbus.write(cmd);
	_i2cbus.endTransmission();
};

/******************************************************************************
 * Read response sent by SDI12 device
 * @return Bytes read from adapter
 *****************************************************************************/
int SDI12Adapter::read()
{
	memset(_buff, 0, SDI12_ADAPTER_BUFF_SIZE);

	_i2cbus.requestFrom(SDI12_I2C_ADDRESS, SDI12_ADAPTER_BUFF_SIZE);

	_i2cbus.setTimeOut(SDI12_ADAPTER_READ_TIMEOUT_MS);
	
	int bytes_read = _i2cbus.readBytesUntil('\r', _buff, SDI12_ADAPTER_BUFF_SIZE);

	Serial.print(F("SDI12 received ("));
	Serial.print(bytes_read);
	Serial.print(" b): ");
	Serial.println(_buff);
}

/******************************************************************************
 * Calculate CRC of the current data in buffer
 * CRC is last 3 chars. Can contain non printable characterss
 *****************************************************************************/
bool SDI12Adapter::check_crc()
{
	int data_length = strlen(_buff);

	// No point in calculating CRC if there's no data
	if(data_length < 4)
		return false;

	// Extract crc from string
	char input_crc[4] = "";
	strncpy(input_crc, (const char*)&_buff[data_length-3], 3);
	input_crc[3] = '\0';

	// Serial.print(F("Data: "));
	// Serial.println(_buff);
	// Serial.print(F("CRC: "));
	// Serial.println(input_crc);

	// Calculate
	uint16_t calced_crc = 0;

	for (int i = 0; i < data_length - 3; i++)
	{
		calced_crc = _buff[i] ^ calced_crc;

		for (int j = 1; j <= 8; j++)
		{
			if (calced_crc & 0x01)
			{
				calced_crc = calced_crc >> 1;
				calced_crc = 0xA001 ^ calced_crc;
			}
			else
			{
				calced_crc = calced_crc >> 1;
			}
		}
	}

	char calced_crc_str[4] = "";
	calced_crc_str[0] = 0x40 | (calced_crc >> 12);
	calced_crc_str[1] = 0x40 | ((calced_crc >> 6) & 0x3F);
	calced_crc_str[2] = 0x40 | (calced_crc & 0x3F);

	// Serial.printf("Input CRC: %s - Calculated CRC: %s\n", input_crc, calced_crc_str);

	return strcmp(input_crc, calced_crc_str) == 0;
}

/******************************************************************************
 * Get data buffer
 *****************************************************************************/
const char* SDI12Adapter::get_data()
{
	return _buff;
}