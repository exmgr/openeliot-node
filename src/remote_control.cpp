#include "remote_control.h"
#include "HardwareSerial.h"
#include "water_sensors.h"
#include "device_config.h"
#include "gsm.h"
#include "sleep.h"

/******************************************************************************
 * 
 *****************************************************************************/
namespace RemoteControl
{
	//
	// Private funcs
	//
	RetResult json_to_data_struct(const JsonDocument &json, RemoteControl::Data *data);


	/** Set when reboot command is received in remote control data so device can be rebooted
	 * when calling home handling ends.
	 */
	bool _reboot_pending = false;

	/******************************************************************************
	 * Handle remote control
	 *****************************************************************************/
	RetResult start()
	{
		//
		// Build URL
		//
		char url[URL_BUFFER_SIZE] = "";
		snprintf(url, sizeof(url), "%s/%s", BACKEND_URL, BACKEND_REMOTE_CONTROL_PATH);

		//
		// Send request
		//
		uint16_t status_code = 0, resp_len = 0;
		char resp_buff[REMOTE_CONTROL_DATA_BUFF_SIZE] = "";
		if(GSM::get_req(url, &status_code, &resp_len, resp_buff, sizeof(resp_buff)) != RET_OK)
		{
			Serial.println(F("Could not send request for remote control."));
			return RET_ERROR;
		}
		// GSM::req(url, "POST", )
		// Dummy
		// char resp_buff[] = "{\"id\": 9,\"ws_int\":26,\"ch_int\":51,\"ota\":0,\"rebt\":1,\"dummy\":1}";

		//
		// Deserialize received data
		//
		StaticJsonDocument<REMOTE_CONTROL_JSON_DOC_SIZE> json_remote;
		DeserializationError error = deserializeJson(json_remote, resp_buff, strlen(resp_buff));

		// Could not deserialize
		if(error)
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Could not deserialize received JSON, aborting."));
			Utils::serial_style(STYLE_RESET);
			
			Log::log(Log::REMOTE_CONTROL_PARSE_FAILED);

			return RET_ERROR;
		}

		Serial.println(F("Remote control JSON: "));
		serializeJsonPretty(json_remote, Serial);
		Serial.println();

		// At least id key must be present
		if(!json_remote.containsKey(JSON_KEY_REMOTE_CONTROL_DATA_ID))
		{
			Serial.println(F("Remote control JSON has no id."));
			Log::log(Log::REMOTE_CONTROL_INVALID_FORMAT);
		}

		//
		// Check if ID is new
		// If remote control ID is old, data is ignored
		// ID is new when it is larger than the one stored from the previous remote control
		int new_data_id = (int)json_remote[JSON_KEY_REMOTE_CONTROL_DATA_ID];

		// Get current remote control data
		const RemoteControl::Data *rc_data_current = DeviceConfig::get_remote_control_data();

		Serial.print(F("Remote control data id: Current "));
		Serial.print(rc_data_current->id);
		Serial.print(F(" - Received "));
		Serial.println(new_data_id);

		// Is it new?
		if(new_data_id <= rc_data_current->id)
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Received remote control data is old, ignoring."));
			Utils::serial_style(STYLE_RESET);	
			return RET_OK;
		}

		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("Received new remote control data. Applying..."));
		Utils::serial_style(STYLE_RESET);

		//
		// Store received config
		//
		RemoteControl::Data rc_data_new;
		
		if(json_to_data_struct(json_remote, &rc_data_new) != RET_OK)
		{
			Serial.println(F("Could not parse JSON into rc data struct. Aborting."));
			return RET_ERROR;
		}

		DeviceConfig::set_remote_control_data(&rc_data_new);

		// Commit device config to apply
		if(DeviceConfig::commit() == RET_ERROR)
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Could not commit applied remote control data."));
			Utils::serial_style(STYLE_RESET);

			// If received config cannot be saved it is considered a fatal error, abort
			// If save fails, config will be considered new every time and config will be applied to eternity
			return RET_ERROR;			
		}
		else
		{
			Serial.println(F("Received device config is saved."));
		}

		//
		// Handle/apply
		//

		// Handle user config
		RemoteControl::handle_user_config(rc_data_new);

		// Handle OTA
		RemoteControl::handle_ota(rc_data_new);

		// Handle reboot
		RemoteControl::handle_reboot(rc_data_new);
	}

	/******************************************************************************
	 * User config
	 *****************************************************************************/
	RetResult handle_user_config(Data data)
	{
		//
		// Water sensors measure interval
		//
		Serial.print(F("Water sensors read interval: "));
		Serial.print(F("Current "));
		Serial.println(DeviceConfig::get_wakeup_schedule_reason_int(Sleep::WakeupReason::REASON_READ_WATER_SENSORS));
		Serial.print(F(" - New "));
		Serial.println(data.water_sensors_measure_int_mins);

		// Check if value within valid range		
		if(WaterSensors::is_measure_int_value_valid(data.water_sensors_measure_int_mins))
		{
			DeviceConfig::set_wakeup_schedule_reason_int(Sleep::WakeupReason::REASON_READ_WATER_SENSORS, data.water_sensors_measure_int_mins);

			Serial.println("New value applied.");
		}
		else
		{
			Utils::serial_style(STYLE_RED);
			Serial.println("Value not valid, ignoring.");
			Utils::serial_style(STYLE_RESET);
		}

		//
		// Call home interval
		//
		
		Serial.print(F("Calling home interval: "));
		Serial.print(F("Current "));
		Serial.println(DeviceConfig::get_wakeup_schedule_reason_int(Sleep::WakeupReason::REASON_CALL_HOME));
		Serial.print(F(" - New: "));
		Serial.println(data.call_home_int_mins);

		if(WaterSensors::is_measure_int_value_valid(data.call_home_int_mins))
		{
			DeviceConfig::set_wakeup_schedule_reason_int(Sleep::WakeupReason::REASON_CALL_HOME, data.call_home_int_mins);

			Serial.println("New value applied.");
		}
		else
		{
			Utils::serial_style(STYLE_RED);
			Serial.println("Value not valid, ignoring.");
			Utils::serial_style(STYLE_RESET);
		}

		return RET_OK;
	}

	/******************************************************************************
	 * Do OTA
	 *****************************************************************************/
	RetResult handle_ota(Data data)
	{
		if(data.ota)
		{
			Utils::serial_style(STYLE_BLUE);
			Serial.println(F("OTA requested."));
			Utils::serial_style(STYLE_RESET);
		}

		// TODO: Do OTA here (write to older of the two partitions)
		// Then mark RemoteControl::reboot_pending() so device reboots when calling home is finished

		return RET_OK;
	}

	/******************************************************************************
	 * Mark device pending flag so device reboots when calling home handling ends
	 *****************************************************************************/
	RetResult handle_reboot(Data data)	
	{	
		if(data.reboot)
		{
			Utils::serial_style(STYLE_BLUE);
			Serial.println(F("Reboot requested. Device will be rebooted when calling home handling ends."));
			Utils::serial_style(STYLE_RESET);

			set_reboot_pending(true);
		}

		return RET_OK;
	}

	/******************************************************************************
	* Get/set reboot pending flag
	* When a reboot is required after calling home handling finishes (eg. to apply
	* some of the required settings), it must set this flag to make CallingHome
	* reboot the device when its done.
	******************************************************************************/
	bool get_reboot_pending()
	{
		return _reboot_pending;
	}
	void set_reboot_pending(bool val)
	{
		_reboot_pending = val;
	}

	/******************************************************************************
	 * Convert received 
	 *****************************************************************************/
	RetResult json_to_data_struct(const JsonDocument &json, RemoteControl::Data *data)
	{
		// ID (required)
		if(!json.containsKey(JSON_KEY_REMOTE_CONTROL_DATA_ID))
		{
			Serial.println(F("Could not parse rc JSON. Invalid format."));

			return RET_ERROR;
		}

		// Zero struct
		memset(data, 0, sizeof(RemoteControl::Data));

		data->id = (int)json[JSON_KEY_REMOTE_CONTROL_DATA_ID];

		// Water sensors measfure interval
		if(json.containsKey(JSON_KEY_MEASURE_WATER_SENSORS_INT))
		{
			data->water_sensors_measure_int_mins = (int)json[JSON_KEY_MEASURE_WATER_SENSORS_INT];
		}

		// Call home interval
		if(json.containsKey(JSON_KEY_CALL_HOME_INT))
		{
			data->call_home_int_mins = (int)json[JSON_KEY_CALL_HOME_INT];
		}

		// Reboot
		if(json.containsKey(JSON_KEY_REBOOT))
		{
			data->reboot = (bool)json[JSON_KEY_REBOOT];
		}

		// OTA
		if(json.containsKey(JSON_KEY_OTA))
		{
			data->ota = (bool)json[JSON_KEY_OTA];
		}

		return RET_OK;
	}

	/******************************************************************************
	* Print data
	******************************************************************************/
	void print(const Data *data)
	{
		Serial.print(F("ID: "));
		Serial.println(data->id, DEC);

		Serial.print(F("Water sensors measure int (mins): "));
		Serial.println(data->water_sensors_measure_int_mins, DEC);

		Serial.print(F("Call home int(mins): "));
		Serial.println(data->call_home_int_mins, DEC);

		Serial.print(F("Reboot: "));
		Serial.println(data->reboot, DEC);

		Serial.print(F("OTA: "));
		Serial.println(data->ota, DEC);
	}
}