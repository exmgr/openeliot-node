#include "call_home.h"
#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#include "log.h"
#include "data_store_reader.h"
#include "sensor_data.h"
#include "rtc.h"
#include "tb_sensor_data_json_builder.h"
#include "tb_diagnostics_json_builder.h"
#include "tb_log_json_builder.h"
#include "test_utils.h"
#include "utils.h"
#include "gsm.h"
#include "flash.h"
#include "device_config.h"
#include "remote_control.h"
#include "battery.h"
#include "int_env_sensor.h"
#include "http_request.h"
#include "log.h"
#include "globals.h"

namespace CallHome
{
	RetResult handle_sensor_data();
	RetResult submit_tb_telemetry(const char *data, int data_size);

	/******************************************************************************
	* Handle waking up from sleep to call home
	******************************************************************************/
	RetResult start()
	{
		RTC::print_time();

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Calling Home"));
		Utils::serial_style(STYLE_RESET);
		Serial.println();

		Log::log(Log::Code::CALLING_HOME);
		Battery::log_adc();
		IntEnvSensor::log();

		Log::log(Log::Code::FS_SPACE, SPIFFS.usedBytes(), SPIFFS.totalBytes() - SPIFFS.usedBytes());

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("FILES BEFORE CALLING HOME"));
		Flash::ls();
		Utils::serial_style(STYLE_RESET);

		GSM::on();
		if(GSM::connect_persist() != RET_OK)
		{

			Serial.println(F("Could not connect GSM. Aborting."));
			return RET_ERROR;
		}

		// Log GSM battery
		Battery::log_gsm();

		// Log RSSI
		Log::log(Log::GSM_RSSI, GSM::get_rssi());

		//
		// Send sensor data
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Submitting sensor data"));
		Utils::serial_style(STYLE_RESET);
		handle_sensor_data();

		//
		// Send logs
		//

		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Submitting logs"));
		Utils::serial_style(STYLE_RESET);
		handle_logs();

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("FILES AFTER CALLING HOME"));
		Flash::ls();
		Utils::serial_style(STYLE_RESET);

		//
		// Ask for remote control data and apply
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Request remote control data"));
		Utils::serial_style(STYLE_RESET);
		handle_remote_control();

		//
		// Publish TB client attributes
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Publishing TB client attributes"));
		Utils::serial_style(STYLE_RESET);
		handle_client_attributes();

		// Done

		GSM::off();

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Calling Home END"));
		Utils::serial_style(STYLE_RESET);
		Serial.println();

		//
		// Calling home handling finished, reboot if requested.
		//
		if(RemoteControl::get_reboot_pending())
		{
			Utils::serial_style(STYLE_BLUE);
			Serial.println(F("Device reboot requested. Rebooting..."));
			Utils::serial_style(STYLE_RESET);

			Utils::restart_device();
		}

		return RET_OK;
	}
	
	/******************************************************************************
	 * Handle sensor data submission
	 * Read all sensor data, break into requests of X entries and submit
	 *****************************************************************************/
	RetResult handle_sensor_data()
	{
		// TODO: If X requests fail, abort completely to avoid failing on 1000 reqs when
		// there is connectivity downtime

		// Entries in current request packet
		int cur_req_entries = 0;
		// Keep count of failed CRCs for log
		int crc_failures = 0;
		// Total sensor data entries
		int total_entries = 0;
		// Total entries submitted (valid entries)
		int submitted_entries = 0;
		// Total successfull entries
		int successfull_entries = 0;
		// Total number of requests 
		int total_requests = 0;
		// Number of successfull requests
		int successfull_requests = 0;

		// Output buffer for resulting JSON
		char json_buff[SENSOR_DATA_JSON_OUTPUT_BUFF_SIZE] = {0};
		
		DataStoreReader<SensorData::Entry> reader(SensorData::get_store());
		const SensorData::Entry *entry = NULL;
		TbSensorDataJsonBuilder tb_json;

		//
		// Iterate all data and submit. Each file in flash will fit in a single request.
		// If request succeedes, file is deleted, if not it is left to be retried next time.
		//
		while(reader.next_file())
		{
			cur_req_entries = 0;

			// Iterate all file entries
			while(entry = reader.next_entry())
			{
				total_entries++;
				if(!reader.entry_crc_valid())
				{
					crc_failures++;
					continue;
				}
				
				cur_req_entries++;
				submitted_entries++;

				tb_json.add(entry);
			}

			// Send only if there are valid entries to be sent
			if(cur_req_entries > 0)
			{
				tb_json.build(json_buff, SENSOR_DATA_JSON_OUTPUT_BUFF_SIZE, false);

				total_requests++;

				if(submit_tb_telemetry(json_buff, strlen(json_buff)) == RET_OK)
				{
					// Request success, file can be deleted
					reader.delete_file();

					Utils::serial_style(STYLE_BLUE);
					Serial.println(F("Deleting file, all complete"));
					Utils::serial_style(STYLE_RESET);

					successfull_entries += cur_req_entries;

					successfull_requests++;
				}
				else
				{
					Utils::serial_style(STYLE_RED);
					Serial.println(F("Sending data failed. File remains to be retried next time."));
					Utils::serial_style(STYLE_RESET);
				}
			}
			else
			{
				// All entries failed CRC in this file so it is useless, delete it
				reader.delete_file();

				Utils::serial_style(STYLE_BLUE);
				Serial.println(F("Deleting file, BAD CRC"));
				Utils::serial_style(STYLE_RESET);
			}

			// Empty packet and prepare for next
			tb_json.reset();
		}

		// Print report
		int entries_failed_submission = submitted_entries - successfull_entries;
		int failed_requests = total_requests - successfull_requests;

		Serial.print(F("Total entries: "));
		Serial.println(total_entries, DEC);
		Serial.print(F("Submitted entries: "));
		Serial.println(submitted_entries, DEC);
		Serial.print(F("Sucessful entries: "));
		Serial.println(successfull_entries, DEC);
		Serial.print(F("Entries failed CRC32: "));
		Serial.println(crc_failures, DEC);
		Serial.println();

		Log::log(Log::SENSOR_DATA_SUBMITTED, submitted_entries, crc_failures);

		// Log only if errors occurred
		if(failed_requests > 0)
			Log::log(Log::SENSOR_DATA_SUBMISSION_ERRORS, total_requests, failed_requests);

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Sensor data submission complete."));
		Utils::serial_style(STYLE_RESET);

		// TODO: When does this fail??
		return RET_OK;
	}

	/******************************************************************************
	 * Handle logs submission
	 * Read all logs, break into requests of X entries and submit
	 *****************************************************************************/
	RetResult handle_logs()
	{
		// Entries in current request packet
		int cur_req_entries = 0;
		// Keep count of failed CRCs for log
		int crc_failures = 0;
		// Total sensor data entries
		int total_entries = 0;
		// Total entries submitted (valid entries)
		int submitted_entries = 0;
		// Total successfull entries
		int successfull_entries = 0;

		// Output buffer for resulting log JSON
		char json_buff[LOG_JSON_OUTPUT_BUFF_SIZE] = {0};

		DataStoreReader<Log::Entry> reader(Log::get_store());
		const Log::Entry *entry = NULL;
		TbLogJsonBuilder tb_log_json;
		TbDiagnosticsJsonBuilder tb_diag_json;

		// Disable logs to avoid getting stuck in loop in case an error occurrs while
		// accessing the file system to read the logs
		Log::set_enabled(false);
	
		//
		// Iterate all logs and submit. Each file in flash will fit in a single request.
		// If request succeedes, file is deleted, if not it is left to be retried next time.
		//
		while(reader.next_file())
		{
			cur_req_entries = 0;

			// Iterate all file entries
			while(entry = reader.next_entry())
			{
				total_entries++;
				if(!reader.entry_crc_valid())
				{
					crc_failures++;
					continue;
				}

				cur_req_entries++;
				submitted_entries++;

				tb_log_json.add(entry);

				// Add to tb diagnostics json builder
				if(PUBLISH_TB_DIAGNOSTIC_TELEMETRY)
				{
					tb_diag_json.add(entry);
				}
			}

			// Send logs only if there are valid entries to be sent
			if(cur_req_entries > 0)
			{
				tb_log_json.build(json_buff, LOG_JSON_OUTPUT_BUFF_SIZE, false);

				if(submit_tb_telemetry(json_buff, strlen(json_buff)) == RET_OK)
				{
					// Request success, file can be deleted
					reader.delete_file();

					Utils::serial_style(STYLE_BLUE);
					Serial.println(F("Deleting file, all complete"));
					Utils::serial_style(STYLE_RESET);

					successfull_entries += cur_req_entries;
				}
				else
				{
					Utils::serial_style(STYLE_RED);
					Serial.println(F("Sending data failed. File remains to be retried next time."));
					Utils::serial_style(STYLE_RESET);
				}
			}
			else
			{
				// All entries failed CRC in this file, delete it
				reader.delete_file();

				Utils::serial_style(STYLE_BLUE);
				Serial.println(F("Deleting file, BAD CRC"));
				Utils::serial_style(STYLE_RESET);
			}

			if(PUBLISH_TB_DIAGNOSTIC_TELEMETRY)
			{
				// Send diagnostics logs to tb
				if(!tb_diag_json.is_empty())
				{
					tb_diag_json.build(json_buff, LOG_JSON_OUTPUT_BUFF_SIZE, false);

					Utils::serial_style(STYLE_CYAN);
					Serial.println(F("Submitting diagnostics JSON: "));
					Utils::serial_style(STYLE_RESET);

					// Use same routine as with sensor data since its the same tb device
					if(submit_tb_telemetry(json_buff, strlen(json_buff)) == RET_OK)
					{
						Utils::serial_style(STYLE_BLUE);
						Serial.println(F("Diagnostics submitted to TB."));
						Utils::serial_style(STYLE_RESET);

						successfull_entries += cur_req_entries;
					}
					else
					{
						Utils::serial_style(STYLE_RED);
						Serial.println(F("Could not submit diagnostics to TB."));
						Utils::serial_style(STYLE_RESET);
					}
				}
				else
				{
					Utils::serial_style(STYLE_CYAN);
					Serial.println(F("No diagnostics to send."));
					Utils::serial_style(STYLE_RESET);
				}

				// Empty packet and prepare for next
				tb_log_json.reset();
			}
			

			// Empty packet and prepare for next
			tb_diag_json.reset();
		}

		// Reenable logging
		Log::set_enabled(true);

		// Print report
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Log data submission complete."));
		Utils::serial_style(STYLE_RESET);
		Serial.print(F("Total entries: "));
		Serial.println(total_entries, DEC);
		Serial.print(F("Submitted entries: "));
		Serial.println(submitted_entries, DEC);
		Serial.print(F("Sucessful entries: "));
		Serial.println(successfull_entries, DEC);
		Serial.print(F("Entries failed CRC32: "));
		Serial.println(crc_failures, DEC);
		Serial.println();

		Log::log(Log::LOG_SUBMITTED, total_entries, crc_failures);
		
		// TODO: When does this fail?
		return RET_OK;
	}

	/******************************************************************************
	 * Submit data to the TB telemetry API endpoint
	 * @param data Buffer with json for TB
	 * @param data_size Buffer size
	 *****************************************************************************/
	RetResult submit_tb_telemetry(const char *data, int data_size)
	{
		char url[URL_BUFFER_SIZE] = "";

		// Get TB token to build URL
		const DeviceDescriptor *device = Utils::get_device_descriptor();
		snprintf(url, sizeof(url), TB_TELEMETRY_URL_FORMAT, device->tb_access_token);

		Utils::print_separator(F("Submitting JSON"));
		Serial.println(data);
		Utils::print_separator(F("END JSON"));

		// Send REQ
		HttpRequest http_req(GSM::get_modem(), TB_SERVER);
		http_req.set_port(TB_PORT);

		RetResult ret = http_req.post(url, (uint8_t*)data, data_size, "application/json", NULL, NULL);
		Serial.flush();

		if(ret != RET_OK || http_req.get_response_code() != 200)
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("TB telemetry submission failed."));
			Utils::serial_style(STYLE_RESET);
			return RET_ERROR;
		}

		return RET_OK;
	}

	/******************************************************************************
	 * Request remote config and apply if received any
	 *****************************************************************************/
	RetResult handle_remote_control()
	{
		return RemoteControl::start();
	}

	/******************************************************************************
	 * Publish TB client attributes
	 * Client attributes contain current device data that is not sent as telemetry
	 * eg. fw version, current measure intervals etc.
	 *****************************************************************************/
	RetResult handle_client_attributes()
	{
		StaticJsonDocument<CLIENT_ATTRIBUTES_JSON_DOC_SIZE> json_doc;
		char url[URL_BUFFER_SIZE_LARGE] = "";

		// Devive token required for URL
		const DeviceDescriptor *device = Utils::get_device_descriptor();
		snprintf(url, sizeof(url), TB_CLIENT_ATTRIBUTES_URL_FORMAT, device->tb_access_token);

		// Add keys
		json_doc[TB_ATTR_CUR_FW_V] = FW_VERSION;
		json_doc[TB_ATTR_CUR_WS_INT] = DeviceConfig::get_wakeup_schedule_reason_int(Sleep::REASON_READ_WATER_SENSORS);
		json_doc[TB_ATTR_CUR_CH_INT] = DeviceConfig::get_wakeup_schedule_reason_int(Sleep::REASON_CALL_HOME);

		serializeJson(json_doc, g_resp_buffer, sizeof(g_resp_buffer));

		// Submit request
		Serial.print(F("Submitting client attribute req: "));
		Serial.println(g_resp_buffer);

		HttpRequest http_req(GSM::get_modem(), TB_SERVER);
		http_req.set_port(TB_PORT);
		// TODO: Is it problematic to use same buffer for send/receive?
		RetResult ret = http_req.post(url, (uint8_t*)g_resp_buffer, strlen(g_resp_buffer), "application/json", g_resp_buffer, sizeof(g_resp_buffer));

		if(ret != RET_OK)
		{
			Serial.println(F("Could not publish client attributes."));
			
			Log::log(Log::TB_CLIENT_ATTR_PUBLISH_FAILED, http_req.get_response_code());
		}

		return ret;
	}

	/******************************************************************************
	* Check if call home interval (mins) value is within valid range
	******************************************************************************/
	bool is_call_home_int_value_valid(int interval)
	{
		return (interval >= CALL_HOME_INT_MINS_MIN && interval <= CALL_HOME_INT_MINS_MAX);
	}
} 