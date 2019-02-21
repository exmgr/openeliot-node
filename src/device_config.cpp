#include "device_config.h"
#include "app_config.h"

/******************************************************************************
* Config uses the "Preferences" API (which has own partition in flash)
* to store all values that need to persist between reboots. SPIFFS is not used
* because config must not be lost in case SPIFFS gets corrupted.
*
* All configurable device data is stored in Config.
* Default data is used when no config set yet (first run) and when loading 
* data from Preferences api fails.
******************************************************************************/
namespace DeviceConfig
{
	//
	// Private vars
	//
	/** Default config used when no config set (first boot) or config is corrupted */
	const DeviceConfig::Data DEVICE_CONFIG_DEFAULT = 
	{
		crc32: 0,
		clean_reboot: false,
		wakeup_schedule: {},

		apply_remote_control_on_boot: false,
		remote_control_data: RemoteControl::Data(0, 0, 0, 0)
	};
	
	/** Currently loaded config. Struct is kept up to date every time new config is set */
	Data _current_config = DEVICE_CONFIG_DEFAULT;

	/** Preferences api store */
	Preferences _prefs;

	//
	// Private functions
	//
	RetResult begin();
	RetResult end();
	RetResult load();
	RetResult write_defaults();

	/******************************************************************************
	* Initialization
	* Must be run once on boot to 
	******************************************************************************/
	RetResult init()
	{
		// Try to load config. If failed, we can only assume that key has not been created
		// yet (first run on new device) since reads fail the same way whether the key
		// doesn't exist or for other reason (no way to tell apart)
		if(load() != RET_OK)
		{
			Serial.println(F("Config store key not set "));

			write_defaults();

			// Does load still fail? If yes, the key not existing wasn't the problem.
			if(load() != RET_OK)
			{
				// Load failed but the defaults have already been written to the
				// current config struct and will be used.
				Serial.println(F("Could not load config. Init failed. Defaults will be used."));
				return RET_ERROR;
			}
		}

		return RET_OK;
	}

	/******************************************************************************
	* Begin NVS. Must be called before every read/write procedure
	******************************************************************************/
	RetResult begin()
	{
		// Close any pending handles before beginning new
		end();
		
		if(!_prefs.begin(DEVICE_CONFIG_NVS_NAMESPACE_NAME))
		{
			end();
			Serial.println(F("Could not begin config NVS store."));
			return RET_ERROR;
		}
		
		return RET_OK;
	}

	/******************************************************************************
	* End NVS. Must be called after every read/write procedure.
	******************************************************************************/
	RetResult end()
	{
		_prefs.end();
	}

	/******************************************************************************
	* Get currently loaded config.
	******************************************************************************/
	const Data* get()
	{
		RetResult ret = RET_ERROR;

		return &_current_config;
	}

	/******************************************************************************
	* Write config to memory. Must be run every time config is changed to make it
	* persistent.
	******************************************************************************/
	RetResult commit()
	{
		RetResult ret = RET_ERROR;

		if(begin() == RET_ERROR)
		{
			return RET_ERROR;
		}

		// Update CRC32 before writing
		// Calc CRC32 without the crc32 field
		_current_config.crc32 = 0;
		_current_config.crc32 = Utils::crc32((uint8_t*)&_current_config, sizeof(_current_config));

		// Write
		int bytes_written = _prefs.putBytes(DEVICE_CONFIG_NVS_NAMESPACE_NAME, &_current_config, sizeof(_current_config));
		if(bytes_written != sizeof(_current_config))
		{
			Serial.print(F("Could not write config to NVS. Bytes written: "));
			Serial.println(bytes_written, DEC);
			Serial.print(F("Expected: "));
			Serial.println(sizeof(_current_config), DEC);

			ret = RET_ERROR;
		}
		else
		{
			ret = RET_OK;
		}

		end();

		return ret;
	}

	/******************************************************************************
	* Load from flash
	******************************************************************************/
	RetResult load()
	{
		if(begin() != RET_OK)
			return RET_ERROR;

		// Load to temp struct first in case it fails
		Data loaded_config;
		RetResult ret = RET_ERROR;

		int read_bytes = _prefs.getBytes(DEVICE_CONFIG_NVS_NAMESPACE_NAME, &loaded_config, sizeof(loaded_config));

		// Read successful
		if(read_bytes == sizeof(loaded_config))
		{
			// Check crc. CRC is checked with the CRC field = 0, so set to 0 but keep backup first
			uint32_t crc32_bkp = loaded_config.crc32;
			loaded_config.crc32 = 0;

			// CRC check failed. Log event and abort.
			if(crc32_bkp != Utils::crc32((uint8_t*)&loaded_config, sizeof(loaded_config)))
			{
				Serial.println(F("Config failed CRC32 check. Loading aborted."));

				Log::log(Log::CONFIG_DATA_CRC_ERRORS);

				ret = RET_ERROR;
			}
			// All OK. Copy loaded config to current config
			else
			{
				// Restore crc32
				loaded_config.crc32 = crc32_bkp;

				memcpy(&_current_config, &loaded_config, sizeof(_current_config));

				ret = RET_OK;
			}
		}
		// Bytes read, but invalid count.
		else
		{
			Serial.println(F("Could not load Config from flash. Invalid byte count."));
			Serial.print(F("Bytes read: "));
			Serial.println(read_bytes, DEC);
			Serial.print(F("Expected: "));
			Serial.println(sizeof(loaded_config), DEC);
			ret = RET_ERROR;
		}

		end();

		return ret;
	}

	/******************************************************************************
	* Write default config structure to NVS. Runs when NVS cannot be started 
	* because key doesn't exist yet (first boot of device)
	******************************************************************************/
	RetResult write_defaults()
	{
		Serial.println(F("Writing default config..."));

		// Copy defaults struct to current
		memcpy(&_current_config, &DEVICE_CONFIG_DEFAULT, sizeof(DEVICE_CONFIG_DEFAULT));
		// Copy wake up schedule to current
		set_wakeup_schedule((Sleep::WakeupScheduleEntry*) &WAKEUP_SCHEDULE_DEFAULT);

		commit();

		return RET_OK;
	}

	/******************************************************************************
	* Print struct to serial output
	******************************************************************************/
	void print(const Data *data)
	{
		Serial.print(F("CRC32: "));
		Serial.println(data->crc32, DEC);

		Serial.print(F("Clean reboot: "));
		Serial.println(data->clean_reboot, DEC);

		Serial.print(F("Apply remote control: "));
		Serial.println(data->apply_remote_control_on_boot, DEC);

		Serial.println(F("Remote control data:"));
		RemoteControl::print(&(data->remote_control_data));
	}

	/******************************************************************************
	 * Print current config struct
	 *****************************************************************************/
	void print_current()
	{
		print(&_current_config);
	}

	/******************************************************************************
	* Accessors
	******************************************************************************/
	RetResult set_clean_reboot(bool val)
	{
		_current_config.clean_reboot = val;

		return RET_OK;
	}
	bool get_clean_reboot()
	{
		return _current_config.clean_reboot;
	}

	RetResult set_apply_remote_control(bool val)
	{
		_current_config.apply_remote_control_on_boot = val;
		
		return RET_OK;
	}
	RetResult set_wakeup_schedule(const Sleep::WakeupScheduleEntry *schedule)
	{
		memcpy(_current_config.wakeup_schedule, schedule, sizeof(_current_config.wakeup_schedule));
	}

	/******************************************************************************
	* Find a reason in the sleep schedule and update its interval
	******************************************************************************/
	RetResult set_wakeup_schedule_reason_int(Sleep::WakeupReason reason, int int_mins)
	{
		for(int i = 0; i < WAKEUP_SCHEDULE_LEN; i++)
		{
			if(_current_config.wakeup_schedule[i].reason == reason)
			{
				_current_config.wakeup_schedule[i].interval_mins = int_mins;
				return RET_OK;
			}
		}

		// Not found

		return RET_ERROR;
	}

	/******************************************************************************
	* Find a reason in the sleep schedule and update its interval
	******************************************************************************/
	int get_wakeup_schedule_reason_int(Sleep::WakeupReason reason)
	{
		for(int i = 0; i < WAKEUP_SCHEDULE_LEN; i++)
		{
			if(_current_config.wakeup_schedule[i].reason == reason)
			{
				return _current_config.wakeup_schedule[i].reason;
			}
		}

		// Not found
		return -1;
	}

	/******************************************************************************
	* Set remote control data structure
	******************************************************************************/
	RetResult set_remote_control_data(const RemoteControl::Data *new_data)
	{
		memcpy(&_current_config.remote_control_data, new_data, sizeof(RemoteControl::Data));
		return RET_OK;
	}

	const RemoteControl::Data* get_remote_control_data()
	{
		return &_current_config.remote_control_data;
	}

}