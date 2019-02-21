#include "tb_diagnostics_json_builder.h"
#include "log_codes.h"
#include "utils.h"

/******************************************************************************
 * Add packet to request
 *****************************************************************************/
RetResult TbDiagnosticsJsonBuilder::add(const Log::Entry *entry)
{
	if(!is_telemetry_code(entry->code))
	{
		return RET_ERROR;
	}

	JsonObject json_entry = _root_array.createNestedObject();
	json_entry["ts"] = (long long)entry->timestamp * 1000;
	JsonObject values = json_entry.createNestedObject("values");

	// Fill value depending on code
	switch(entry->code)
	{
	case Log::GSM_CONNECT_FAILED:
		values["d_gsm_conn_fail"] = 1;
		break;
	case Log::GSM_RSSI:
		// RSSI in dBm
		values["d_gsm_rssi"] = (int)entry->meta1;
		break;
	case Log::NTP_TIME_SYNC_FAILED:
		values["d_ntp_fail"] = 1;
		break;
	case Log::SENSOR_DATA_SUBMITTED:
		// Total records submitted
		values["d_sd_total_rec"] = (int)entry->meta1;
		
		// Only if there were CRC failures
		if(entry->meta2 > 0)
		{
			values["d_sd_crc"] = (int)entry->meta2;
		}
		break;
	case Log::SENSOR_DATA_SUBMISSION_ERRORS:
		// Total requests
		values["d_sd_total_req"] = (int)entry->meta1;
		// Failed reqs
		values["d_sd_failed_req"] = (int)entry->meta2;
		break;
	case Log::BATTERY:
		// Voltage
		values["d_bat_mv"] = (int)entry->meta1;
		// Pct
		values["d_bat_p"] = (int)entry->meta2;
		break;
	case Log::BATTERY_GSM:
		// Voltage
		values["d_bat_gsm_mv"] = (int)entry->meta1;
		// Pct
		values["d_bat_gsm_p"] = (int)entry->meta2;
		break;
	case Log::INT_ENV_SENSOR1:
		// Temperature
		values["d_temp"] = (float)entry->meta1;
		// Humidity
		values["d_hum"] = (float)entry->meta2;
		break;
	case Log::SLEEP:
		// Time awake since last sleep
		values["d_tm_awake_s"] = (int)entry->meta1;
		break;
	case Log::WAKEUP:
		values["d_wakeup"] = 1;
		break;
	case Log::BOOT:
		values["d_boot"] = 1;
		break;
	default:
		Utils::serial_style(STYLE_RED);
		Serial.println(F("Unknown key"));
		Utils::serial_style(STYLE_RESET);
	}

	return RET_OK;
}


/******************************************************************************
 * Check if provided log code needs to be sent as telemetry to TB
 ******************************************************************************/
bool TbDiagnosticsJsonBuilder::is_telemetry_code(Log::Code code)
{
	// All valid telemetry log codes
	Log::Code valid_codes[] = {
		Log::BOOT,
		Log::GSM_CONNECT_FAILED,
		Log::GSM_RSSI,
		Log::NTP_TIME_SYNC_FAILED,
		Log::SLEEP,
		Log::WAKEUP,
		Log::SENSOR_DATA_SUBMISSION_ERRORS,
		Log::SENSOR_DATA_SUBMITTED,
		Log::BATTERY,
		Log::BATTERY_GSM,
		Log::INT_ENV_SENSOR1
	};

	// Check 
	int items = sizeof(valid_codes) / sizeof(valid_codes[0]);
	for (int i = 0; i < items; i++)
	{
		// Found
		if(valid_codes[i] == code)
		{
			return true;
		}
	}

	// Not found
	return false;
}