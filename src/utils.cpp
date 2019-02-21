#include <Arduino.h>
#include "utils.h"
#include "SPIFFS.h"
#include "app_config.h"
#include "struct.h"
#include "CRC32.h"

namespace Utils
{
	/********************************************************************************
	 * Get ESP32 mac address
	 * @param out Output buffer. Must be at least 18 bytes in size
	 * @param size String size
	 ********************************************************************************/
	void get_mac(char *out, uint8_t size)
	{
		uint8_t mac[6];
		
		esp_read_mac(mac, ESP_MAC_WIFI_STA);
		
		snprintf(out, size,  "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], out[5]);
	}

	/********************************************************************************
	 * Get current device's device descriptor
	 * Device descriptors are mapped to devices by their MAC
	 * Not fidning the device is a fatal condition. 
	 ********************************************************************************/
	const DeviceDescriptor* get_device_descriptor()
	{
		DeviceDescriptor *descriptor = NULL;
		
		// Total devices
		uint8_t devices = sizeof(DEVICE_DESCRIPTORS) / sizeof(DEVICE_DESCRIPTORS[0]);

		// Current device mac
		char current_mac[18] = "";
		get_mac(current_mac, sizeof(current_mac));

		for(uint8_t i = 0; i < devices; i++)
		{
			if(strcmp(DEVICE_DESCRIPTORS[i].mac, current_mac) == 0)
				return &DEVICE_DESCRIPTORS[i];
		}

		if(descriptor == NULL)
		{
			Serial.print(F("ACHTUNG! Device not recognized! MAC: "));
			Serial.println(current_mac);
			Utils::serial_style(STYLE_WHITE_BKG);
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Terminating."));

			while(1){}
		}

		// Line never reached, has to be here for compiler error
		return NULL;
	}

	/******************************************************************************
	* Get battery level
	* @return Battery level percentage
	******************************************************************************/
	int battery_level()
	{
		// TODO: Stub. Currently returns dummy value

		return 87;
	}


	/********************************************************************************
	 * Print a separator line to the debug output with an optional name/title
	 * Helps with debugging
	 * @param name Text to display inside separator
	 ********************************************************************************/
	void print_separator(const __FlashStringHelper *title)
	{
		const int WIDTH = 55;
		Serial.print(F("\x18\x18\x18\x18"));

		int padding = 0;		

		if(title == NULL)
		{
			// Total WIDTH chars will be printed
			padding = WIDTH - 4;
		}
		else
		{
			// 5 chars for #### with space, text length, 1 space
			padding = WIDTH - 5 - strlen((char*)title) - 1;
			if(padding < 0)
			{
				// Add 4 chars in any case
				padding = 4;
			}

			// Space before and after title
			Serial.print(F(" "));
			Serial.print(F(title));
			Serial.print(F(" "));
		}

		// Fill padding with chars
		for(int i = 0; i < padding; i++)
		{
			Serial.print('\x18');
		}

		Serial.println();
	}

	/********************************************************************************
	 * Print a separator block with a title
	 * Helps with debugging
	 * @param title Text to display inside block
	 ********************************************************************************/
	void print_block(const __FlashStringHelper *title)
	{
		const int WIDTH = 55;
		Serial.println("=======================================================");
		Serial.printf("= %-*s%s \n", WIDTH - 3, title, "=");
		Serial.println("=======================================================");
	}

	/********************************************************************************
	 * Set console output style
	 * @param style A combination of styles from Style enum
	 *******************************************************************************/
	void serial_style(SerialStyle style)
	{
		Serial.printf("\033[%dm", style);
	}

    /********************************************************************************
	 * Calculate CRC32 of a data buffer
	 * @param data Data packet
	 *******************************************************************************/
	uint32_t crc32(uint8_t *buff, uint32_t buff_size)
	{
		return CRC32::calculate((uint8_t*)buff, buff_size);
	}

}