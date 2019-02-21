#include "flash.h"
#include "SPIFFS.h"
#include "utils.h"
#include "const.h"
#include "struct.h"

namespace Flash
{
	/********************************************************************************
	* Mount SPIFFS partition
	*******************************************************************************/
	RetResult mount()
	{
		int tries = 2;
		bool success = false;

		while(tries--)
		{
			if(SPIFFS.begin())
			{
				success = true;
				break;
			}
			else
			{
				Serial.println(F("Could not mount SPIFFS."));
				if(tries > 1)
					Serial.println(F("Retrying..."));					
			}
		}

		// If mounting failed, format partition then try mounting again
		if(!success)
		{
			Serial.println(F("Could not mount SPIFFS, formatting partition..."));
			SPIFFS.format();

			if(SPIFFS.begin())
			{
				Serial.println(F("Partition mount successful."));
				return RET_OK;
			}
			else
			{
				Utils::serial_style(STYLE_RED);
				Serial.println(F("Mounting failed."));
				Utils::serial_style(STYLE_RESET);	
				return RET_ERROR;
			}
		}

		return RET_OK;
	}

	/********************************************************************************
	* Mount partition and read a file into a buffer
	*******************************************************************************/
	RetResult read_file(const char *path, uint8_t *dest, int bytes)
	{
		if(!SPIFFS.begin())
		{
			Serial.print(F("Could not mount flash."));
			return RET_ERROR;
		}

		File f = SPIFFS.open(path, FILE_READ);

		// File doesn't exist
		if(!f)
		{
			Serial.println(F("Could not load file."));
			return RET_ERROR;
		}

		// Try to read file
		if(bytes != f.read(dest, bytes))
		{
			Serial.println(F("Could not read file."));
			return RET_ERROR;
		}

		return RET_OK;
	}

	/********************************************************************************
	* Print all files and dirs in flash
	*******************************************************************************/
	void ls()
	{
		Utils::print_separator(F("Flash memory contents"));

		// if(!SPIFFS.begin())
		// {
		//     Serial.println(F("Could not begin SPIFFS."));
		//     return;
		// }

		File root = SPIFFS.open("/");
		if(!root)
		{
			Serial.println(F("Could not open root."));
			return;
		}

		File cur_file;
		
		while(cur_file = root.openNextFile())
		{
			Serial.print(cur_file.name());

			// Print size
			Serial.print(F(" ["));
			Serial.print(cur_file.size());
			Serial.print(F("]"));
			Serial.println();
		}

		Utils::print_separator(F("End flash memory contents"));
	}
}