#include <Arduino.h>
#include "utils.h"
#include "SPIFFS.h"
#include "app_config.h"
#include "struct.h"
#include "CRC32.h"
#include "Wire.h"
#include "const.h"
#include "device_config.h"
#include "rom/rtc.h"

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

	/******************************************************************************
	 * Set TCall PMIC config
	 *****************************************************************************/
	RetResult ip5306_set_power_boost_state(bool enable)
	{
		// This function has been repuprosed to enable/disable input power (from 4V4) for the
		// 12v stepup that powers the arduino nano and ultrasonic 3v3 
		// TODO rename? split in to two ?
		// (power boost needs no change before sleep)
        
		Serial.println(F("Setting IP5306 power registers"));

		Wire.beginTransmission(IP5306_I2C_ADDR);
		Wire.write(IP5306_REG_SYS_CTL0);
	    Wire.write(IP5306_BOOST_FLAGS); 
   		Wire.endTransmission() ;
		
		if (enable)
		{
			// Enable SY8089 4V4 for SIM800 which is also powering 12V Stepup
			pinMode(PIN_GSM_POWER_ON, OUTPUT);
			digitalWrite(PIN_GSM_POWER_ON, HIGH);
		}
		else 
		{
			pinMode(PIN_GSM_POWER_ON, OUTPUT);
			digitalWrite(PIN_GSM_POWER_ON, LOW);
		}
	
		//TODO check success?
		return RET_OK;


		// if(Wire.requestFrom(IP5306_I2C_ADDR, 1))
		// {
		// 	data = Wire.read();

		// 	Wire.beginTransmission(IP5306_I2C_ADDR);
		// 	Wire.write(IP5306_REG_SYS_CTL0);

		// 	if(enable)
		// 		Wire.write(data |  IP5306_BOOST_OUT_BIT); 
		// 	else
		// 		Wire.write(data & (~IP5306_BOOST_OUT_BIT));  

		// 	Wire.endTransmission();

		// 	Serial.println(F("Setting power boost state successful."));
		// 	return RET_OK;
		// }

		// Utils::serial_style(STYLE_RED);
		// Serial.println(F("Setting power boost state failed."));
		// Utils::serial_style(STYLE_RESET);

		// return RET_ERROR;;
	}

	/******************************************************************************
	 * Explode URL into host, port and path
	 * Fails if not BOTH host and path parts are found (port is optional)
	 *****************************************************************************/
	RetResult url_explode(char *in, int *port_out, char *host_out, int host_max_size, char *path_out, int path_max_size)
	{
		char *cursor = in;
		
		// Check if there's a protocol and ignore
		char *proto_start = strstr(cursor, "//");
		if(proto_start != nullptr)
		{
			// Ignore protocol part
			cursor = proto_start + 2;
		}
		
		char *host_end = strchr(cursor, ':');
		char *port_start = nullptr;
		
		// No : found, so host will be up to the next /, find it
		if(host_end == nullptr)
		{
			host_end = strchr(cursor, '/');
		}
		else
		{
			// Host end found and there's a port part too
			port_start = host_end;
		}

		// No next / found means host ends at end of string but also means there's no resource path, abort
		if(host_end != nullptr)
		{
			int host_len = host_end - cursor;
			if(host_len > host_max_size)
			{
				Serial.println(F("Host output buffer too small."));
				return RET_ERROR;
			}
				
			strncpy(host_out, cursor, host_len);
			
			// Set cursor
			cursor = host_end;
		}
		
		// Get port if it exists
		if(port_start != nullptr)
		{		
			char port_str[6] = "";
			
			char *port_end = strchr(port_start, '/');
			if(port_end != nullptr)
			{
				char port_str[6] = "";
				
				strncpy(port_str, port_start, 5);
				sscanf(port_start+1, "%5d", port_out);
				
			}
			else
			{
				// Port end not found
				return RET_ERROR;
			}
			
			cursor = port_end;
		}
		
		// The rest is path
		// If its only a single char (a '/'), there's no path, abort
		int path_len = strlen(cursor);
		if(path_len < 2)
		{
			return RET_ERROR;
		}
		else if(path_len > path_max_size)
		{
			Serial.println(F("Path output buffer too small."));
			return RET_ERROR;
		}
		
		strncpy(path_out, cursor, path_max_size);
		
		return RET_OK;
	}

	/******************************************************************************
	 * Build TB attributes API URL for this device
	 *****************************************************************************/
	// TODO: Useless?? Can be inline
	RetResult tb_build_attributes_url_path(char *buff, int buff_size)
	{
		// Devive token required for URL
		const DeviceDescriptor *device = Utils::get_device_descriptor();

		snprintf(buff, buff_size, TB_SHARED_ATTRIBUTES_URL_FORMAT, device->tb_access_token);

		return RET_OK;
	}

	/******************************************************************************
	 * Build TB telemetry API URL for this device
	 *****************************************************************************/
	RetResult tb_build_telemetry_url_path(char *buff, int buff_size)
	{
		// Devive token required for URL
		const DeviceDescriptor *device = Utils::get_device_descriptor();

		snprintf(buff, buff_size, TB_TELEMETRY_URL_FORMAT, device->tb_access_token);

		return RET_OK;
	}
	
	/******************************************************************************
	 * Mark restart as clean and restart
	 *****************************************************************************/
	RetResult restart_device()
	{
		DeviceConfig::set_clean_reboot(true);
		DeviceConfig::commit();

		ESP.restart();
	}

	/******************************************************************************
	 * Print reset reason
	 *****************************************************************************/
	void print_reset_reason()
	{
		RESET_REASON reason = rtc_get_reset_reason(0);

		switch(reason)
		{
			case RESET_REASON::POWERON_RESET:
				Serial.println(F("Power ON")); 
				break;
			case RESET_REASON::SW_RESET:
				Serial.println(F("Software reset (digital core)"));
				break;
			case RESET_REASON::OWDT_RESET:
				Serial.println(F("Legacy watchdog (digital core)"));
				break;
			case RESET_REASON::DEEPSLEEP_RESET:
				Serial.println(F("Deep sleep (digital core)"));
				break;
			case RESET_REASON::SDIO_RESET:
				Serial.println(F("SLC module (digital core)"));
				break;
			case RESET_REASON::TG0WDT_SYS_RESET:
				Serial.println(F("TG0 Watchdog (digital core)"));
				break;
			case RESET_REASON::TG1WDT_SYS_RESET:
				Serial.println(F("TG1 Watchdog (digital core)"));
				break;
			case RESET_REASON::RTCWDT_SYS_RESET:
				Serial.println(F("RTC Watchdog (digital core)"));
				break;
			case RESET_REASON::INTRUSION_RESET:
				Serial.println(F("Intrusion tested to reset CPU"));
				break;
			case RESET_REASON::TGWDT_CPU_RESET:
				Serial.println(F("Timer group watchdog (CPU)"));
				break;
			case RESET_REASON::SW_CPU_RESET:
				Serial.println(F("Software reset (CPU)"));
				break;
			case RESET_REASON::RTCWDT_CPU_RESET:
				Serial.println(F("RTC Watchdog (CPU)"));
				break;
			case RESET_REASON::EXT_CPU_RESET:
				Serial.println(F("Ext (CPU)"));
				break;
			case RESET_REASON::RTCWDT_BROWN_OUT_RESET:
				Serial.println(F("Brown out"));
				break;
			case RESET_REASON::RTCWDT_RTC_RESET:
				Serial.println(F("RTC watch dog reset (digital core and rtc module)"));
				break;
		}
	}
}