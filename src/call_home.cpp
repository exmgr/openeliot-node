#include "call_home.h"
#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#include "log.h"
#include "data_store_reader.h"
#include "sensor_data.h"
#include "rtc.h"
#include "tb_sensor_data_json_builder.h"
#include "tb_diagnostics_json_builder.h"
#include "log_json_builder.h"
#include "test_utils.h"
#include "utils.h"
#include "gsm.h"
#include "flash.h"
#include "device_config.h"
#include "remote_control.h"
#include "battery.h"
#include "int_env_sensor.h"

namespace CallHome
{
	RetResult handle_sensor_data();
	RetResult submit_sensor_data(const char *data, int data_size);

	RetResult handle_logs();
	RetResult submit_log_data(const char *data, int data_size);

	/******************************************************************************
	* Handle waking up from sleep to call home
	******************************************************************************/
	RetResult start()
	{
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Calling Home"));
		Utils::serial_style(STYLE_RESET);
		Serial.println();

		Log::log(Log::Code::CALLING_HOME);
		Battery::log_gauge();
		IntEnvSensor::log();

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
		// Send device status and receive config
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Request remote control data"));
		Utils::serial_style(STYLE_RESET);
		// handle_remote_control();

		GSM::off();

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Calling Home END"));
		Utils::serial_style(STYLE_RESET);
		Serial.println();

		//
		// Send diagnostics telemetry to TB
		//
		// TODO: Useless???
		//handle_diagnostics();

		//
		// Calling home handling finished, reboot if requested.
		//
		if(RemoteControl::get_reboot_pending())
		{
			// Set flag before rebooting so we know that the reboot was normal
			DeviceConfig::set_clean_reboot(true);
			DeviceConfig::commit();

			Utils::serial_style(STYLE_BLUE);
			Serial.println(F("Device reboot requested. Rebooting..."));
			Utils::serial_style(STYLE_RESET);

			ESP.restart();
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

				if(submit_sensor_data(json_buff, strlen(json_buff)) == RET_OK)
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
				// All entries failed CRC in this file, delete it
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
	 * Submit a single sensor data request to TB
	 * @param data Buffer with json for TB
	 * @param data_size Buffer size
	 *****************************************************************************/
	RetResult submit_sensor_data(const char *data, int data_size)
	{
		char url[URL_BUFFER_SIZE] = "";

		TestUtils::print_stack_size();

		// Get TB token to build URL
		const DeviceDescriptor *device = Utils::get_device_descriptor();
		snprintf(url, sizeof(url), TB_TELEMETRY_URL_FORMAT, TB_URL, device->tb_access_token);

		Utils::print_separator(F("Submitting JSON"));
		Serial.println(data);
		Utils::print_separator(F("END JSON"));
		
		// Send REQ
		RetResult ret = GSM::req(url, "POST", data, data_size, NULL, 0);
		Serial.flush();

		return ret;
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
		LogJsonBuilder log_json;
		TbDiagnosticsJsonBuilder tb_diag_json;
		
		//
		// Iterate all logs and submit. Each file in flash will fit in a single request.
		// If request succeedes, file is deleted, if not it is left to be retried next time.
		//
		// NOTE: Using the log (eg. Log::log) while data is read from log, can cause unexpected
		// behaviour. Do not use the log until after reading has finished.
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

				log_json.add(entry);

				// Add to tb diagnostics json builder
				tb_diag_json.add(entry);
			}

			// Send logs only if there are valid entries to be sent
			if(cur_req_entries > 0)
			{
				log_json.build(json_buff, LOG_JSON_OUTPUT_BUFF_SIZE, false);

				if(submit_log_data(json_buff, strlen(json_buff)) == RET_OK)
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

			// Send diagnostics logs to tb
			if(!tb_diag_json.is_empty())
			{
				tb_diag_json.build(json_buff, LOG_JSON_OUTPUT_BUFF_SIZE, false);

				Utils::serial_style(STYLE_CYAN);
				Serial.print(F("Diag JSON: "));
				Serial.println(json_buff);
				Utils::serial_style(STYLE_RESET);

				// Use same routine as with sensor data since its the same tb device
				if(submit_sensor_data(json_buff, strlen(json_buff)) == RET_OK)
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
			log_json.reset();
			tb_diag_json.reset();
		}

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
	 * Submit a single log data request to backend
	 * @param data Buffer
	 * @param data_size Buffer size
	 *****************************************************************************/
	RetResult submit_log_data(const char *data, int data_size)
	{
		char url[URL_BUFFER_SIZE] = "";

		// Get device id to build URL
		const DeviceDescriptor *device = Utils::get_device_descriptor();
		snprintf(url, sizeof(url), BACKEND_LOG_URL_FORMAT, BACKEND_URL, device->id);
		
		// Send REQ
		RetResult ret = GSM::req(url, "POST", data, data_size);

		return ret;
	}

	/******************************************************************************
	 * Submit diagnostic data to thingsboard
	 *****************************************************************************/
	RetResult handle_diagnostics()
	{
		// StaticJsonDocument<DIAGNOSTICS_JSON_DOC_SIZE> json_doc;
		// char out_buff[DIAGNOTICS_JSON_OUTPUT_BUFF_SIZE] = "";

		// json_doc.clear();
		// JsonArray root_array = json_doc.template to<JsonArray>();

		// JsonObject json_entry = root_array.createNestedObject();
		// json_entry["ts"] = (long long)RTC::get_timestamp();
		// JsonObject values = json_entry.createNestedObject("values");
		
		// values["a"] = 1;
		// values["b"] = 2;
		// values["c"] = 3;
		// values["d"] = 4;

		// serializeJsonPretty(json_doc, out_buff, sizeof(out_buff));



		// Serial.println(F("Result JSON"));
		// Serial.print(out_buff);
	
	}

	/******************************************************************************
	 * Request remote config and apply if received any
	 *****************************************************************************/
	RetResult handle_remote_control()
	{
		return RemoteControl::start();
	}

	/******************************************************************************
	* Check if call home interval (mins) value is within valid range
	******************************************************************************/
	bool is_call_home_int_value_valid(int interval)
	{
		return (interval >= CALL_HOME_INT_MINS_MIN && interval <= CALL_HOME_INT_MINS_MAX);
	}
} 