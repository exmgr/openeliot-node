#include "ota.h"
#include "utils.h"
#include "log.h"
#include "test_utils.h"
#include "TinyGsmClient.h"
#include "gsm.h"
#include "ArduinoHttpClient.h"
#include "Update.h"
#include "globals.h"
#include "remote_control.h"
#include "http_request.h"
#include "device_config.h"

namespace OTA
{
	/******************************************************************************
	 * Handle request for OTA
	 * @param json JSON data received from remote control request
	 *****************************************************************************/
	RetResult handle_rc_data(JsonObject rc_json)
	{
		// Do OTA?
		if(!rc_json.containsKey(TB_KEY_DO_OTA))
		{
			return RET_ERROR;
		}
		else
		{
			bool do_ota = bool(rc_json[TB_KEY_DO_OTA]);
			if(!do_ota)
				return RET_ERROR;
		}

		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("OTA requested."));
		Utils::serial_style(STYLE_RESET);

		Log::log(Log::OTA_REQUESTED);

		// Version must be different than current
		if(!rc_json.containsKey(TB_KEY_FW_VERSION))
		{
			Serial.println(F("No FW version specified in response."));

			Log::log(Log::OTA_NO_FW_VERSION_SPECIFIED, FW_VERSION);

			return RET_ERROR;
		}
		else
		{
			int fw_version = (int)rc_json[TB_KEY_FW_VERSION];

			if(fw_version == FW_VERSION)
			{
				Serial.println(F("OTA fw version is same as current."));
				Serial.print(F("Current version: "));
				Serial.println(FW_VERSION, DEC);

				Log::log(Log::OTA_FW_SAME_VERSION, FW_VERSION);

				return RET_ERROR;
			}
		}

		// Get URL to download OTA from
		char fw_url[URL_BUFFER_SIZE] = "";
		if(rc_json.containsKey(TB_KEY_FW_URL) && strlen(rc_json[TB_KEY_FW_URL]) > 0)
		{
			strncpy(fw_url, rc_json[TB_KEY_FW_URL], sizeof(fw_url));
		}
		else
		{
			Serial.println(F("OTA fw url not provided or empty."));

			Log::log(Log::OTA_FW_URL_NOT_SET);
			return RET_ERROR;
		}

		// Get MD5 to validate against
		char fw_md5[33] = "";
		if(rc_json.containsKey(TB_KEY_FW_MD5) && strlen(rc_json[TB_KEY_FW_MD5]) > 0)
		{
			strncpy(fw_md5, rc_json[TB_KEY_FW_MD5], sizeof(fw_md5));
		}
		else
		{
			Serial.println(F("OTA fw md5 not provided or empty."));

			Log::log(Log::OTA_MD5_NOT_SET);
			return RET_ERROR;
		}

		//
		// Do request
		//
		Serial.println(F("Getting OTA file."));

		// Break URL into parts
		int port = 0;
		char fw_url_host[URL_HOST_BUFFER_SIZE] = "";
		char fw_url_path[URL_BUFFER_SIZE] = "";
		if(Utils::url_explode(fw_url, &port, fw_url_host, sizeof(fw_url_host), fw_url_path, sizeof(fw_url_path)) == RET_ERROR)
		{
			Serial.println(F("Invalid FW URL."));

			Log::log(Log::OTA_URL_INVALID);
			return RET_ERROR;
		}

		// Use default port if not provided
		if(port == 0)
		{
			port = 80;
		}

		Serial.print(F("Host: "));
		Serial.println(fw_url_host);
		Serial.print(F("Port: "));
		Serial.println(port, DEC);
		Serial.print(F("Path: "));
		Serial.println(fw_url_path);

		TestUtils::print_stack_size();
		
		TinyGsmClient client(*GSM::get_modem());		
		HttpClient http_client(client, (char*)fw_url_host, port);
		
		int req_ret = http_client.get(fw_url_path);
		if(req_ret != 0)
		{
			Serial.println(F("Could not GET fw."));

			Log::log(Log::OTA_FILE_GET_REQ_FAILED, http_client.responseStatusCode());
			return RET_ERROR;
		}

		int response_code = http_client.responseStatusCode();
		int content_length = http_client.contentLength();
		Serial.print(F("Response code: "));
		Serial.println(response_code, DEC);
		Serial.print(F("Update size: "));
		Serial.println(http_client.contentLength(), DEC);
		
		if(response_code != 200)
		{
			Serial.println(F("Request did not return OK."));

			Log::log(Log::OTA_FILE_GET_REQ_BAD_RESPONSE, response_code);
			return RET_ERROR;
		}

		if(http_client.contentLength() < 1)
		{
			Serial.println(F("Response empty. Aborting."));

			Log::log(Log::OTA_FILE_GET_REQ_RESP_EMPTY, response_code);
			return RET_ERROR;
		}

		//
		// Read response and write to update memory
		//
		if(Update.begin(content_length) == false)
		{
			Serial.println(F("Could not begin update. Aborting."));

			Log::log(Log::OTA_UPDATE_BEGIN_FAILED, Update.getError());
			return RET_ERROR;
		}

		// Set received MD5 for validation
		Update.setMD5(fw_md5);

		// Stream read timeout
		http_client.setTimeout(HTTP_CLIENT_STREAM_READ_TIMEOUT);

		Serial.println(F("Downloading and writing flash..."));

		Log::log(Log::OTA_DOWNLOADING_AND_WRITING_FW, content_length);

		//
		// Receive data from GSM module and write partition
		// 
		int bytes_read = 0, bytes_remaining = content_length;

		Utils::serial_style(STYLE_BLUE);
		do
		{
			int bytes_to_read = bytes_remaining > GLOBAL_HTTP_RESPONSE_BUFFER_LEN ? GLOBAL_HTTP_RESPONSE_BUFFER_LEN : bytes_remaining;

			// Read into global response buffer
			bytes_read = http_client.readBytes(g_resp_buffer, bytes_to_read);

			Update.write((uint8_t*)g_resp_buffer, bytes_read);


			bytes_remaining -= bytes_read;

			// Move cursor to start of line and print progress

			Serial.print("\e[0A");
			Serial.printf("Progress: %5d%% - Remaining: %5dKB\n", (content_length - bytes_remaining) / (content_length / 100), bytes_remaining / 1024);
			
		}while(bytes_remaining > 0 && bytes_read != 0);

		Serial.println(F("Done writing new fw."));

		Utils::serial_style(STYLE_RESET);

		Log::log(Log::OTA_WRITING_FW_COMPLETE, content_length, content_length - bytes_remaining);

		// Do checks
		if(!Update.isFinished())
		{
			Serial.println(F("Update not finished. Error: "));
			Update.printError(Serial);

			Log::log(Log::OTA_UPDATE_NOT_FINISHED, Update.getError());
			return RET_ERROR;
		}
		else
		{
			if(!Update.end())
			{
				Serial.println(F("Could not end update. Error:"));		
				Update.printError(Serial);

				Log::log(Log::OTA_COULD_NOT_FINALIZE_UPDATE, Update.getError());
				return RET_ERROR;
			}
			else
			{
				Serial.println(F("Update applied. Device will be restarted when ready."));

				Log::log(Log::OTA_FINISHED);
				
				// Set device to reboot when all handling is done
				RemoteControl::set_reboot_pending(true);

				DeviceConfig::set_ota_flashed(true);
				DeviceConfig::commit();
			}
		}		

		return RET_OK;
	}

	/******************************************************************************
	 * Run self-test routines to decide whether OTA was successful. 
	 * To pass a self test, a GET request is sent to the TB server.
	 * If the request fails it most likely means connectivity is lost after the FW
	 * update so the test fails.
	 *****************************************************************************/
	RetResult self_test()
	{
		if(GSM::on() != RET_OK)
			return RET_ERROR;

		if(GSM::connect_persist() != RET_OK)
			return RET_ERROR;

		// Make a GET request to the attributes API for this device
		HttpRequest http_req(GSM::get_modem(), TB_SERVER);
		http_req.set_port(TB_PORT);

		char url[URL_BUFFER_SIZE_LARGE] = "";
		Utils::tb_build_attributes_url_path(url, sizeof(url));

		if(http_req.get(url, g_resp_buffer, GLOBAL_HTTP_RESPONSE_BUFFER_LEN) != RET_OK)
		{
			Serial.println(F("Request to TB attribute API failed."));
			return RET_ERROR;
		}
		else if(http_req.get_response_code() != 200)
		{
			Serial.print(F("Request to TB attribute API returned code: : "));
			Serial.println(http_req.get_response_code(), DEC);

			return RET_ERROR;
		}

		Serial.println(F("OTA self test passed."));
	}

	/******************************************************************************
	 * Ran on first boot after new OTA is flashed
	 *****************************************************************************/
	RetResult handle_first_boot()
	{
		// Mark as handled
		DeviceConfig::set_ota_flashed(false);
		DeviceConfig::commit();

		Utils::print_block(F("New FW - First boot"));

		Serial.println(F("Running self test"));

		// Run self test
		if(self_test() == RET_OK)
		{
			Utils::serial_style(STYLE_GREEN);
			Serial.println(F("Self test passed. OTA was successful."));
			Utils::serial_style(STYLE_RESET);

			Log::log(Log::OTA_SELF_TEST_PASSED);
		}
		else
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Self test failed."));
			Utils::serial_style(STYLE_RESET);

			Log::log(Log::OTA_SELF_TEST_FAILED);

			if(Update.canRollBack())
			{
				Serial.println(F("FW will be rolled back to older version and device will restart."));
				Log::log(Log::OTA_ROLLING_BACK);
				delay(2000);

				Utils::restart_device();
			}
			else
			{
				Serial.println(F("FW cannot be rolled back to the old version."));

				Log::log(Log::OTA_ROLLBACK_NOT_POSSIBLE);

				return RET_ERROR;
			}
		}

		return RET_OK;
	}
}