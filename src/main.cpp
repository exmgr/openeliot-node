#include <Arduino.h>
#include <stdlib.h>
#include <ctime>
#include <sys/time.h>
#include "common.h"
#include "call_home.h"
#include "const.h"
#include "flash.h"
#include "globals.h"
#include "sensor_data.h"
#include "utils.h"
#include "gsm.h"
#include "rtc.h"
#include "log.h"
#include "tests.h"
#include "test_utils.h"
#include "sleep.h"
#include "water_sensors.h"
#include <Wire.h>
#include <RtcDS3231.h>
#include "WiFi.h"
#include "json_builder_base.h"
#include "tb_log_json_builder.h"
#include "tb_sensor_data_json_builder.h"
#include "device_config.h"
#include "water_quality_sensor.h"
#include "tb_diagnostics_json_builder.h"
#include "water_level.h"
#include "battery.h"
#include "int_env_sensor.h"
#include <SPIFFS.h>
#include "http_request.h"
#include "tb_sensor_data_json_builder.h"
#include "ota.h"

/* For testing */
#include "data_store_reader.h"

void battery_sleep_charge();

void setup() 
{
	Serial.begin(115200);
		
	Utils::serial_style(STYLE_BLUE);
	Utils::print_separator(F("BOOTING"));
	Utils::serial_style(STYLE_RESET);
	Serial.print(F("\n\n"));

	//
	// Print on-boot info
	// 
	Utils::serial_style(STYLE_GREEN);
	Serial.print(F("Reset reason: "));
	Serial.println();
	Utils::print_reset_reason();
	Serial.println();
	Utils::serial_style(STYLE_RESET);

	// Configuration
	Utils::print_separator(F("Configuration"));
	Utils::serial_style(STYLE_RED);
	if(NBIOT_MODE)
		Serial.println(F("- NBIoT mode"));

	if(SLEEP_MINS_AS_SECS)
		Serial.println(F("- Sleep minutes treated as seconds"));

	if(MEASURE_DUMMY_WATER_QUALITY)
		Serial.println(F("- Water Quality measurements return dummy values"));

	if(MEASURE_DUMMY_WATER_LEVEL)
		Serial.println(F("- Water Level measurements return dummy values"));

	if(EXTERNAL_RTC_ENABLED)
		Serial.println(F("- External RTC enabled"));

	if(PUBLISH_TB_DIAGNOSTIC_TELEMETRY)
		Serial.println("- Logs will be sent as TB telemetry (diagnostics)");

	if(PRINT_GSM_AT_COMMS)
		Serial.println(F("- AT command output to console enabled."));

	Utils::serial_style(STYLE_RESET);
	Serial.println();

	// Device info
	Utils::print_separator(F("DEVICE INFO"));

	// Board name
	Serial.print(F("Board: "));
	Serial.println(BOARD_NAME);

	Serial.print(F("Version: "));
	Serial.println(FW_VERSION, DEC);

	// Get device info and output
	const DeviceDescriptor *device_descriptor = Utils::get_device_descriptor();
	if(device_descriptor == NULL)
	{
		char mac[18] = "";
		Utils::get_mac(mac, sizeof(mac));

		Serial.print(F("Unknown device. MAC: "));
		Serial.println(mac);
		Serial.println(F("Update device descriptors to continue. Terminating."));
		while(1){}
	}
	else
	{
		Serial.print(F("ID: "));
		Serial.println(device_descriptor->id, DEC);
		Serial.print(F("MAC: "));
		Serial.println(device_descriptor->mac);
		Serial.print(F("TB Access Token: "));
		Serial.println(device_descriptor->tb_access_token);
		Serial.println();
	}

	Serial.print(F("Program size: "));
	Serial.print(ESP.getSketchSize() / 1024, DEC);

	Serial.println(F("KB"));
	Serial.print(F("Free program space: "));
	Serial.print(ESP.getFreeSketchSpace() / 1024, DEC);
	Serial.println(F("KB"));

	Serial.print(F("Free heap: "));
	Serial.print(ESP.getFreeSketchSpace(), DEC);
	Serial.println(F("B"));

	Serial.print(F("CPU freq: "));
	Serial.print(ESP.getCpuFreqMHz());
	Serial.println("MHz");

	Serial.println();

	/******************************************************************************
	* START
	******************************************************************************/
	//////////////////////////////////////////////
	// Run tests
	//////////////////////////////////////////////
	// Tests::TestId tests[] = {
	// 	// Tests::TestId::DATA_STORE,
	// 	Tests::DEVICE_CONFIG
	// };
	// Tests::run(tests, sizeof(tests) / sizeof(tests[0]));

	// // Tests::run_all();///////////////////////
	// while(1){} // Dont go further
	//////////////////////////////////////////////
	/******************************************************************************
	* END
	******************************************************************************/

	// 	//// PIN BLINK
	// int pins[] = {34, 35, 32, 33, 25, 26, 27, 14};

	// for(int i=0; i < sizeof(pins) / sizeof(pins[0]); i++)
	// {
	// 	pinMode(pins[i], OUTPUT);
	// }

	// while(1)
	// {
	// 	for(int i=0; i < sizeof(pins) / sizeof(pins[0]); i++)
	// 	{
	// 		digitalWrite(pins[i], 1);
	// 	}
	// 	delay(500);>
	// 	for(int i=0; i < sizeof(pins) / sizeof(pins[0]); i++)
	// 	{
	// 		digitalWrite(pins[i], 0);
	// 	}
	// 	delay(500);
	// }
	
	//// 


	//
	// Init 
	// Order important

	// Init main I2C1 bus
	Wire.begin(PIN_I2C1_SDA, PIN_I2C1_SCL, 100000);

	#ifdef TCALL_H
		// Turn IP5306 power boost OFF to reduce idle current
		Utils::ip5306_set_power_boost_state(false);
	#endif

	delay(100);
	IntEnvSensor::init();
	Battery::init();
	delay(100);
	Flash::mount();
	Flash::ls();
	DeviceConfig::init();
	RTC::init();
	GSM::init();
	WaterSensors::init();

	// Log boot now that memory has been inited
	Log::log(Log::Code::BOOT, FW_VERSION);

	// Log and print battery
	Battery::log_adc();

	// Device info
	DeviceConfig::print_current();

	// Check if boot is after OTA
	if(DeviceConfig::get_ota_flashed())
	{
		OTA::handle_first_boot();
	}

	//
	// Check if device needs to be in sleep mode in case this is after a brown out
	//
	battery_sleep_charge();

	//////////////////////////////////////////////////////////////////////////
	// TESTING AREA

	// while(1)
	// {
	// 	GSM::update_ntp_time();

		// tm tm_now;
		// // delay(15000);
		// if(GSM::get_time(&tm_now) == RET_OK)
		// {
		// 	Serial.println(F("Got time"));

		// 	Serial.printf("%02d/%02d/%04d - %02d:%02d:%02d\n", tm_now.tm_mday, tm_now.tm_mon, tm_now.tm_year + 1900,
		// 		tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
		// }
		// else
		// {
		// 	Serial.println(F("Could not get time from GSM."));
		// }

	// 	delay(1000);
	// }

	// GSM::off();

	// Serial.println(F("Done"));
	// while(1);
	/////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////
	// if(1)
	// {
	// 	if(GSM::on() != RET_OK)
	// 	{
	// 		Serial.println(F("Could not turn on gsm."));
	// 		while(1);
	// 	}

	// 	if(GSM::connect_persist() != RET_OK)
	// 	{
	// 		Serial.println(F("Could not connect GPRS."));
	// 		while(1);
	// 	}
	// }

	// RemoteControl::start();
	// CallHome::handle_client_attributes();

	// Serial.println(F("Done"));
	// while(1);
	///////////////////////////////////////////////////////////

	// Format SPIFFS
	// SPIFFS.format();

	////// TEST BME280 
	// while(1)
	// {
	// 	float temp = 0, hum = 0;
	// 	int alt = 0, press = 0;
	// 	IntEnvSensor::read(&temp, &hum, &press, &alt);

	// 	Serial.print(F("Temp: "));
	// 	Serial.println(temp);
	// 	Serial.print(F("Hum: "));
	// 	Serial.println(hum);
	// 	Serial.print(F("Alt: "));
	// 	Serial.println(alt, DEC);
	// 	Serial.print(F("Press: "));
	// 	Serial.println(press, DEC);

	// 	Serial.println(F("-------------------------------------"));

	// 	delay(1000);
	// }
	////// END TEST BME280

	////// TEST WATER QUALITY
	// WaterQualitySensor::init();
	// while(1)
	// {
	// 	WaterQualitySensor::on();
	// 	delay(100);

	// 	SensorData::Entry entry;
	// 	WaterQualitySensor::measure(&entry);
		
	// 	SensorData::print(&entry);
	// 	Serial.println(F("-----------------------------------------------------"));

	// 	delay(100);
	// 	WaterQualitySensor::off();
	// 	delay(2000);
	// }
	////// TEST WATER QUALITY END 


	// ////// TEST WATER LEVEL
	// WaterSensors::on();
	// while(1)
	// {
	// 	SensorData::Entry entry;

	// 	WaterLevel::measure(&entry);

	// 	Serial.print(F("Water level: "));
	// 	Serial.println(entry.water_level);
		
	// 	delay(1000);
	// }
	// WaterSensors::off();
	// ////// TEST WATER LEVEL END

	//
	// Check if reboot clean
	// Reboot is considered clean when the clean_boot flag is set
	// Otherwise the device was hard-reset or reboot was unexpected (exception)
	//
	if(DeviceConfig::get_clean_reboot())
	{
		Utils::serial_style(STYLE_GREEN);
		Serial.println(F("Clean boot"));
		Utils::serial_style(STYLE_RESET);

		// Reset flag
		DeviceConfig::set_clean_reboot(false);
		DeviceConfig::commit();
	}
	else
	{
		Utils::serial_style(STYLE_RED);
		Serial.println(F("Boot is not clean (not intentional)"));
		Utils::serial_style(STYLE_RESET);
	}

	// For CCID, RTC
	if(GSM::on() != RET_ERROR)
	{
		// 
		// Get SIM CCID to check for its presence
		// 
		if(!GSM::sim_card_present())
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("No SIM card detected!"));
			Utils::serial_style(STYLE_RESET);

			Log::log(Log::GSM_NO_SIM_CARD);
		}

		//
		// Sync RTC
		//
		if(RTC::sync() != RET_OK) // RTC turns GSM ON
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Failed to sync time, system has no source of time."));
			Utils::serial_style(STYLE_RESET);
		}
		else
		{
			Serial.println(F("Time sync successful."));
			RTC::print_time();
		}
	}
	
	GSM::off();

	//////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////
	// Log time sync event
	Log::log(Log::Code::TIME_SYNC);

	// TODO: Make all tasks run on boot and remove this
	Utils::serial_style(STYLE_MAGENTA);
	Serial.println(F("Reason: Read sensors"));
	Utils::serial_style(STYLE_RESET);
	WaterSensors::log();
	
	Utils::serial_style(STYLE_BLUE);
	Serial.println(F("Reason: Call home"));
	Utils::serial_style(STYLE_RESET);
	CallHome::start();
	Utils::print_separator(F("Setup finished"));

	Utils::print_separator(F("SETUP COMPLETE"));
}

/******************************************************************************
 * Wake up self test
 * On each wake up, a self test checks if main parameters to see if it is OK
 * to proceed with normal operation (measure, send data etc.)
 * If not, regular wake up is not executed and the device goes back to sleep
 * until next wake up event.
 *****************************************************************************/
RetResult wakeup_self_test()
{
	// Check if RTC returns invalid value
	if(RTC::get_timestamp() <= FAIL_CHECK_TIMESTAMP)
	{
		Utils::serial_style(STYLE_RED);
		Serial.println(F("RTC returns invalid timestamp, syncing..."));
		Utils::serial_style(STYLE_RESET);

		//
		// Sync RTC
		//	
		if(RTC::sync() != RET_OK)
		{
			Serial.println(F("Failed to sync time."));
			return RET_ERROR;
		}
		else
		{
			Serial.println(F("Time sync successful."));
			RTC::print_time();
		}
	}

	return RET_OK;
}

/******************************************************************************
 * Battery is low, device goes into sleep charge mode where it sleeps and all
 * functions are disabled until battery is over the charged threshold.
 * Device wakes up from sleep every X mins to check.
 *****************************************************************************/
void battery_sleep_charge()
{
	if(Battery::get_current_mode() != BATTERY_MODE::BATTERY_MODE_SLEEP_CHARGE)
		return;

	Serial.println(F("Battery critical, going into sleep charge mode."));
	Log::log(Log::SLEEP_CHARGE);
	Battery::log_adc();

	// Set sleep time and go to sleep
	int time_to_sleep_ms = SLEEP_CHARGE_CHECK_INT_MINS * 60000;

	if(SLEEP_MINS_AS_SECS)
	 	time_to_sleep_ms /= 60;

	esp_sleep_enable_timer_wakeup(time_to_sleep_ms * 1000);

	while(true)
	{
		Serial.print(F("Sleeping for (sec): "));
		Serial.println(time_to_sleep_ms / 1000);
		Serial.flush();
		esp_light_sleep_start();
		Serial.println(F("Wake up"));

		uint16_t mv = 0, pct = 0;
		Battery::read_adc(&mv, &pct);		

		if(pct < BATTERY_LEVEL_SLEEP_RECHARGED)
		{
			Serial.println(F("Battery level not quite there yet... Going back to sleep."));
		}
		else
		{
			Serial.println(F("Battery charged up to threshold. Exiting sleep charge mode."));

			Log::log(Log::SLEEP_CHARGE_FINISHED);
			Battery::log_adc();
			break;
		}
	}
}

/******************************************************************************
* Main loop
******************************************************************************/
void loop()
{
	// Handle battery sleep charge if needed
	if(Battery::get_current_mode() == BATTERY_MODE::BATTERY_MODE_SLEEP_CHARGE)
	{
		battery_sleep_charge();
	}

	Sleep::sleep();

	// Do wake up self test
	if(wakeup_self_test() != RET_OK)
	{
		Utils::serial_style(STYLE_RED);
		Serial.println(F("Wake up self test failed, going back to sleep."));
		Utils::serial_style(STYLE_RESET);
	}
	else
	{
		//
		// Tasks executed only when wakeup self test passed
		//

		if(Sleep::wakeup_reason_is(Sleep::REASON_READ_WATER_SENSORS))
		{
			Utils::serial_style(STYLE_MAGENTA);
			Serial.println(F("Reason: Read sensors"));
			Utils::serial_style(STYLE_RESET);
			WaterSensors::log();
		}	
	}
	
	if(Sleep::wakeup_reason_is(Sleep::REASON_CALL_HOME))
	{
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("Reason: Call home"));
		Utils::serial_style(STYLE_RESET);
		CallHome::start();
	}

	Serial.println(F("------------------------------------------------"));
}