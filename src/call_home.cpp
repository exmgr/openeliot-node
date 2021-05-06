#include "call_home.h"
#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#include "common.h"
#include "log.h"
#include "data_store_reader.h"
#include "water_sensor_data.h"
#include "soil_moisture_data.h"
#include "sdi12_log.h"
#include "rtc.h"
#include "tb_water_sensor_data_json_builder.h"
#include "tb_atmos41_data_json_builder.h"
#include "tb_soil_moisture_data_json_builder.h"
#include "tb_sdi12_log_json_builder.h"
#include "tb_fo_data_json_builder.h"
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
#include "atmos41_data.h"
#include "fo_sniffer.h"
#include "fo_uart.h"
#include "fo_buffer.h"

namespace CallHome
{
	//
	// Private functions
	//
	template <typename TStore, typename TBuilder, typename TEntry>
	RetResult submit_stored_telemetry(TStore *store, DataStoreSubmitStats *stats);
	RetResult submit_tb_telemetry(const char *data, int data_size);
	uint32_t build_flags_bitmask();
	RetResult end();

	/******************************************************************************
	* Handle waking up from sleep to call home
	******************************************************************************/
	RetResult start()
	{
		RTC::print_time();

		Utils::serial_style(STYLE_YELLOW);
		Utils::print_separator(F("Calling Home"));
		Utils::serial_style(STYLE_RESET);
		debug_println();

		Log::log(Log::Code::CALLING_HOME);

		// TEMP Log current schedule
		Log::log(Log::SCHEDULE_CALL_HOME_INT, DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::WakeupReason::REASON_CALL_HOME));
		Log::log(Log::SCHEDULE_WATER_SENSORS_INT, DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::WakeupReason::REASON_READ_WATER_SENSORS));
		Log::log(Log::SCHEDULE_WEATHER_STATION_INT, DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::WakeupReason::REASON_READ_WEATHER_STATION));
		Log::log(Log::SCHEDULE_FO_INT, DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::WakeupReason::REASON_FO));
		Log::log(Log::SCHEDULE_SOIL_MOISTURE_INT, DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::WakeupReason::REASON_READ_SOIL_MOISTURE_SENSOR));
		//

		if(FLAGS.FUEL_GAUGE_ENABLED)
		{
			Battery::log_gauge();
		}	
		
		Battery::log_adc();
		Battery::log_solar_adc();

		Log::log(Log::Code::FS_SPACE, SPIFFS.usedBytes(), SPIFFS.totalBytes() - SPIFFS.usedBytes());

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("FILES BEFORE CALLING HOME"));
		Flash::ls();
		Utils::serial_style(STYLE_RESET);

		GSM::on();
		if(GSM::connect_persist() != RET_OK)
		{
			debug_println(F("Could not connect GSM. Aborting."));
			end();
			
			return RET_ERROR;
		}

		// Log RSSI
		Log::log(Log::GSM_RSSI, GSM::get_rssi());

		//
		// Ask for remote control data and apply
		//
		Utils::serial_style(STYLE_BLUE);
		debug_println(F("# Request remote control data"));
		Utils::serial_style(STYLE_RESET);
		if(handle_remote_control() != RET_OK)
		{
			if(RemoteControl::get_last_error() == RemoteControl::ERROR_REQUEST_FAILED)
			{
				debug_println(F("Call home aborted"));
				end();
				return RET_ERROR;
			}
		}

		//
		// Publish TB client attributes
		//
		Utils::serial_style(STYLE_BLUE);
		debug_println(F("# Publishing TB client attributes"));
		Utils::serial_style(STYLE_RESET);
		handle_client_attributes();

		//
		// If reboot requested during remote control, reboot
		//
		if(RemoteControl::get_reboot_pending())
		{
			Utils::serial_style(STYLE_BLUE);
			debug_println(F("Device reboot requested. Rebooting..."));
			Utils::serial_style(STYLE_RESET);

			end();

			Utils::restart_device();
		}

		//
		// Commit FO sniffer data before submitting telemetry
		//
		if(FO_SOURCE == FO_SOURCE_SNIFFER)
		{
			FoSniffer::commit_buffer();
		}
		else if(FO_SOURCE == FO_SOURCE_UART)
		{
			FoUart::commit_buffer();
		}

		//
		// Submit all telemetry from data store
		//
		Utils::serial_style(STYLE_BLUE);
		debug_println(F("# Submitting telemetry"));
		Utils::serial_style(STYLE_RESET);
		handle_telemetry();

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("FILES AFTER CALLING HOME"));
		Flash::ls();
		Utils::serial_style(STYLE_RESET);

		// Done
		end();

		return RET_OK;
	}

	/******************************************************************************
	 * Clean up after finishing
	 *****************************************************************************/
	RetResult end()
	{
		GSM::off();
		
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Calling Home END"));
		Utils::serial_style(STYLE_RESET);
		debug_println();

		Log::log(Log::CALLING_HOME_END);
		Log::log(Log::Code::FS_SPACE, SPIFFS.usedBytes(), SPIFFS.totalBytes() - SPIFFS.usedBytes());

		return RET_OK;
	}

	/******************************************************************************
	 * Handle sensor data submission
	 * Read all sensor data, break into requests of X entries and submit
	 *****************************************************************************/
	RetResult handle_telemetry()
	{
		DataStoreSubmitStats stats = {0};

		// Keep track of time elapsed
		uint32_t telemetry_start_millis = millis();

		//
		// Submit water sensor data
		//
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Submitting water sensor data."));
		Utils::serial_style(STYLE_RESET);

		submit_stored_telemetry<DataStore<WaterSensorData::Entry>, TbWaterSensorDataJsonBuilder, WaterSensorData::Entry>(WaterSensorData::get_store(), &stats);

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Water sensor data submission complete"));
		Utils::serial_style(STYLE_RESET);

		//
		// Submit weather data
		//
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Submitting weather data."));
		Utils::serial_style(STYLE_RESET);

		submit_stored_telemetry<DataStore<Atmos41Data::Entry>, TbAtmos41DataJsonBuilder, Atmos41Data::Entry>(Atmos41Data::get_store(), &stats);

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Weather data submission complete"));
		Utils::serial_style(STYLE_RESET);

		//
		// Submit soil moisture sensor data
		//
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Submitting soil moisture data."));
		Utils::serial_style(STYLE_RESET);

		submit_stored_telemetry<DataStore<SoilMoistureData::Entry>, TbSoilMoistureDataJsonBuilder, SoilMoistureData::Entry>(SoilMoistureData::get_store(), &stats);

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Soil moisture data submission complete"));
		Utils::serial_style(STYLE_RESET);
		
		//
		// Submit FO data
		//
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Submitting FineOffset weather data."));
		Utils::serial_style(STYLE_RESET);

		submit_stored_telemetry<DataStore<FoData::StoreEntry>, TbFoDataJsonBuilder, FoData::StoreEntry>(FoData::get_store(), &stats);

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("FineOffset weather data submission complete"));
		Utils::serial_style(STYLE_RESET);


		//
		// Submit SDI12 debug data
		//
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Submitting SDI12 debug data."));
		Utils::serial_style(STYLE_RESET);

		submit_stored_telemetry<DataStore<SDI12Log::Entry>, TbSDI12LogJsonBuilder, SDI12Log::Entry>(SDI12Log::get_store(), &stats);

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("SDI12 debug data submission complete"));
		Utils::serial_style(STYLE_RESET);

		uint32_t telemetry_elapsed_sec = (millis() - telemetry_start_millis) / 1000;
		

		//
		// Submit logs
		//
		uint32_t logs_start_millis = millis();

		handle_logs();

		uint32_t logs_elapsed_sec = (millis() - logs_start_millis) / 1000;

		//
		// Print stats
		//

		Utils::print_separator(F("Overall stats"));

		debug_print(F("Total entries: "));
		debug_println(stats.total_entries, DEC);
		debug_print(F("Submitted entries: "));
		debug_println(stats.submitted_entries, DEC);
		debug_print(F("Successful entries: "));
		debug_println(stats.successful_entries, DEC);
		debug_print(F("Entries failed CRC: "));
		debug_println(stats.crc_failed_entries, DEC);
		debug_print(F("Total requests: "));
		debug_println(stats.total_requests, DEC);
		debug_print(F("Failed requests: "));
		debug_println(stats.failed_requests, DEC);
		debug_println();
		debug_print(F("Telemetry took (sec): "));
		debug_println(telemetry_elapsed_sec, DEC);
		debug_print(F("Logs took (sec): "));
		debug_println(logs_elapsed_sec, DEC);
		debug_println();

		Log::log(Log::SENSOR_DATA_SUBMITTED, stats.submitted_entries, stats.crc_failed_entries);

		Log::log(Log::DATA_SUBMISSION_ELAPSED, telemetry_elapsed_sec, logs_elapsed_sec);

		// Log only if errors occurred
		if(stats.failed_requests > 0)
			Log::log(Log::SENSOR_DATA_SUBMISSION_ERRORS, stats.total_requests, stats.failed_requests);

		Utils::print_separator(NULL);

		return RET_OK;
	}

	/******************************************************************************
	 * Read all data from a DataStore, build JSON and submit as telemetry
	 *****************************************************************************/
	template <typename TStore, typename TBuilder, typename TEntry>
	RetResult submit_stored_telemetry(TStore *store, DataStoreSubmitStats *stats)
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
		// Total number of requests 
		int total_requests = 0;
		// Number of successfull requests
		int successfull_requests = 0;

		TBuilder json_builder;

		// Output buffer for resulting JSON
		char json_buff[TELEMETRY_DATA_JSON_OUTPUT_BUFF_SIZE] = {0};
		
		DataStoreReader<TEntry> reader(store);
		const TEntry *entry = NULL;

		json_builder.reset();

		//
		// Iterate all data and submit. Each file in flash will fit in a single request.
		// If request succeeds, file is deleted, if not it is left to be retried next time.
		//
		while(reader.next_file())
		{
			cur_req_entries = 0;

		// Iterate all file entries
			while((entry = reader.next_entry()))
			{
				total_entries++;
				if(!reader.entry_crc_valid())
				{
					crc_failures++;
					continue;
				}
				
				cur_req_entries++;
				submitted_entries++;

				json_builder.add(entry);
			}

			// Send only if there are valid entries to be sent
			if(cur_req_entries > 0)
			{
				json_builder.build(json_buff, sizeof(json_buff), false);

				total_requests++;

				if(submit_tb_telemetry(json_buff, strlen(json_buff)) == RET_OK)
				{
					// Request success, file can be deleted
					reader.delete_file();

					Utils::serial_style(STYLE_BLUE);
					debug_println(F("Deleting file, all complete"));
					Utils::serial_style(STYLE_RESET);

					successfull_entries += cur_req_entries;
					successfull_requests++;
				}
				else
				{
					Utils::serial_style(STYLE_RED);
					debug_println(F("Sending telemetry data failed. File remains to be retried next time."));
					Utils::serial_style(STYLE_RESET);
				}
			}
			else
			{
				// All entries failed CRC in this file so it is useless, delete it
				reader.delete_file();

				Utils::serial_style(STYLE_BLUE);
				debug_println(F("Deleting file, BAD CRC"));
				Utils::serial_style(STYLE_RESET);
			}

			// Empty packet and prepare for next
			json_builder.reset();
		}

		// Print report
		int failed_requests = total_requests - successfull_requests;

		debug_print(F("Total entries: "));
		debug_println(total_entries, DEC);
		debug_print(F("Submitted entries: "));
		debug_println(submitted_entries, DEC);
		debug_print(F("Successful entries: "));
		debug_println(successfull_entries, DEC);
		debug_print(F("Entries failed CRC32: "));
		debug_println(crc_failures, DEC);
		debug_println();

		// Output operation stats (add to provided)
		if(stats != nullptr)
		{
			stats->total_entries += total_entries;
			stats->submitted_entries += submitted_entries;
			stats->successful_entries += successfull_entries;
			stats->crc_failed_entries += crc_failures;
			stats->total_requests += total_requests;
			stats->failed_requests += failed_requests;
		}

		// TODO: Under what conditions this function is considered failed??
		return RET_OK;
	}

	/******************************************************************************
	 * Submit all logs
	 * @param data Buffer with json for TB
	 * @param data_size Buffer size
	 *****************************************************************************/
	RetResult handle_logs()
	{
		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Submitting logs."));
		Utils::serial_style(STYLE_RESET);

		// Disable logs to avoid getting stuck in loop in case an error occurrs while
		// accessing the file system to read the logs
		Log::set_enabled(false);

		submit_stored_telemetry<DataStore<Log::Entry>, TbLogJsonBuilder, Log::Entry>(Log::get_store(), nullptr);

		// Reenable logging
		Log::set_enabled(true);

		Utils::serial_style(STYLE_BLUE);
		Utils::print_separator(F("Log submission complete"));
		Utils::serial_style(STYLE_RESET);

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

		snprintf(url, sizeof(url), TB_TELEMETRY_URL_FORMAT, DeviceConfig::get_tb_device_token());

		Utils::print_separator(F("Submitting JSON"));
		debug_println(data);
		Utils::print_separator(F("END JSON"));

		// Send REQ
		HttpRequest http_req(GSM::get_modem(), TB_SERVER);
		http_req.set_port(TB_PORT);

		RetResult ret = http_req.post(url, (uint8_t*)data, data_size, "application/json", NULL, 0);
		Serial.flush();

		if(ret != RET_OK || http_req.get_response_code() != 200)
		{
			Utils::serial_style(STYLE_RED);
			debug_println(F("TB telemetry submission failed."));
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

		// Device token required for URL
		snprintf(url, sizeof(url), TB_CLIENT_ATTRIBUTES_URL_FORMAT, DeviceConfig::get_tb_device_token());

		//
		// Add main keys
		//
		json_doc[TB_ATTR_CUR_FW_V] = FW_VERSION;
		json_doc[TB_ATTR_CUR_WAS_INT] = DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::REASON_READ_WATER_SENSORS);
		json_doc[TB_ATTR_CUR_WES_INT] = DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::REASON_READ_WEATHER_STATION);
		json_doc[TB_ATTR_CUR_SM_INT] = DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::REASON_READ_SOIL_MOISTURE_SENSOR);
		json_doc[TB_ATTR_CUR_CH_INT] = DeviceConfig::get_wakeup_schedule_reason_int(SleepScheduler::REASON_CALL_HOME);
		json_doc[TB_ATTR_CUR_SYSTEM_TIME] = RTC::get_timestamp();
		json_doc[TB_ATTR_UPTIME] = millis() / 1000;
		json_doc[TB_ATTR_FLAGS] = build_flags_bitmask();

		// FO Enabled
		json_doc[TB_ATTR_CUR_FO_EN] = DeviceConfig::get_fo_enabled();

		// FO id
		char fo_id[5] = "";
		snprintf(fo_id, sizeof(fo_id), "%02x", DeviceConfig::get_fo_sniffer_id());
		json_doc[TB_ATTR_CUR_FO_ID] = fo_id;

		// Include aquatroll model if water quality sensor is enabled
		if(FLAGS.WATER_QUALITY_SENSOR_ENABLED)
		{
			if(AQUATROLL_MODEL == AQUATROLL_MODEL_400)
			{
				json_doc[TB_ATTR_AQUATROLL_MODEL] = "400";
			}
			else if(AQUATROLL_MODEL == AQUATROLL_MODEL_500)
			{
				json_doc[TB_ATTR_AQUATROLL_MODEL] = "500";
			}
			else if(AQUATROLL_MODEL == AQUATROLL_MODEL_600)
			{
				json_doc[TB_ATTR_AQUATROLL_MODEL] = "600";
			}
			else
			{
				json_doc[TB_ATTR_AQUATROLL_MODEL] = "";
			}
			json_doc[TB_ATTR_UPTIME] = millis() / 1000;
		}

		//
		// Add flags 
		//
		
		serializeJson(json_doc, g_resp_buffer, sizeof(g_resp_buffer));

		// Submit request
		debug_print(F("Submitting client attribute req: "));
		debug_println(g_resp_buffer);

		HttpRequest http_req(GSM::get_modem(), TB_SERVER);
		http_req.set_port(TB_PORT);
		// TODO: Is it problematic to use same buffer for send/receive?
		RetResult ret = http_req.post(url, (uint8_t*)g_resp_buffer, strlen(g_resp_buffer), "application/json", g_resp_buffer, sizeof(g_resp_buffer));

		if(ret != RET_OK)
		{
			debug_println(F("Could not publish client attributes."));
			
			Log::log(Log::TB_CLIENT_ATTR_PUBLISH_FAILED, http_req.get_response_code());
		}

		return ret;
	}

	
	/******************************************************************************
	 * Build a bitmask from FLAGS to submit as attribute to server
	 * Compiler messes up bit order so it must be done here manually
	 *****************************************************************************/
	uint32_t build_flags_bitmask()
	{
		uint32_t bits = 0;

		bits =
			FLAGS.DEBUG_MODE |
			(FLAGS.LOG_RAW_SDI12_COMMS << 1) | 
			(FLAGS.WIFI_DEBUG_CONSOLE_ENABLED << 2) | 
			(FLAGS.WIFI_DATA_SUBMISSION_ENABLED << 3) | 
			(FLAGS.FUEL_GAUGE_ENABLED << 4) | 
			(FLAGS.NBIOT_MODE << 5) | 
			(FLAGS.SLEEP_MINS_AS_SECS << 6) | 
			(FLAGS.BATTERY_FORCE_NORMAL_MODE << 7) | 
			(FLAGS.WATER_QUALITY_SENSOR_ENABLED << 8) | 
			(FLAGS.WATER_LEVEL_SENSOR_ENABLED << 9) | 
			(FLAGS.WEATHER_STATION_ENABLED << 10) | 
			(FLAGS.SOIL_MOISTURE_SENSOR_ENABLED << 12) | 
			(FLAGS.MEASURE_DUMMY_WATER_QUALITY << 13) | 
			(FLAGS.MEASURE_DUMMY_WATER_LEVEL << 14) | 
			(FLAGS.MEASURE_DUMMY_WEATHER << 15) | 
			(FLAGS.EXTERNAL_RTC_ENABLED << 16);

		return bits;
	}

	/******************************************************************************
	* Check if call home interval (mins) value is within valid range
	******************************************************************************/
	bool is_call_home_int_value_valid(int interval)
	{
		return (interval >= CALL_HOME_INT_MINS_MIN && interval <= CALL_HOME_INT_MINS_MAX);
	}
} 