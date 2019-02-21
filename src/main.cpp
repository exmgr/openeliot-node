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
#include "log_json_builder.h"
#include "tb_sensor_data_json_builder.h"
#include "device_config.h"
#include "water_quality_sensor.h"
#include "tb_diagnostics_json_builder.h"
#include "water_level.h"
#include "battery.h"
#include "int_env_sensor.h"
#include <SPIFFS.h>

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

	// Configuration
	Utils::print_separator(F("Configuration"));
	Utils::serial_style(STYLE_RED);
	if(NBIOT_MODE)
		Serial.println("- NBIoT mode");

	if(SLEEP_MINS_AS_SECS)
		Serial.println("- Sleep minutes treated as seconds");

	if(MEASURE_DUMMY_WATER_QUALITY)
		Serial.println("- Water Quality measurements return dummy values");

	if(MEASURE_DUMMY_WATER_LEVEL)
		Serial.println("- Water Level measurements return dummy values");

	if(USE_EXTERNAL_RTC)
		Serial.println("- External RTC enabled");

	Utils::serial_style(STYLE_RESET);
	Serial.println();

	// Device info
	Utils::print_separator(F("DEVICE INFO"));

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

	//
	// Set pins
	//
	// GSM power enable
	pinMode(PIN_GSM_PWR, OUTPUT);

	// Water level analog input, power enable
	pinMode(PIN_WATER_LEVEL_ANALOG, ANALOG);
	pinMode(PIN_WATER_LEVEL_PWR, OUTPUT);

	// Water quality power enable
	pinMode(PIN_WATER_QUALITY_PWR, OUTPUT);


	//
	// Init 
	// Order important

	// Init main I2C1 bus
	Wire.begin(PIN_I2C1_SDA, PIN_I2C1_SCL, 100000);

	delay(100);
	IntEnvSensor::init();
	Battery::init();
	delay(100);
	Flash::mount();
	Flash::ls();
	DeviceConfig::init();
	RTC::init();
	GSM::init();
	WaterLevel::init();

	// Device info
	Utils::print_separator(F("CURRENT DEVICE CONFIG"));
	DeviceConfig::print_current();

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
	// while(1)
	// {
	// 	WaterLevel::on();

	// 	SensorData::Entry entry;

	// 	WaterLevel::measure(&entry);

	// 	Serial.print(F("Water level: "));
	// 	Serial.println(entry.water_level);
		
	// 	WaterLevel::off();
	// 	delay(1000);
	// }
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
		// /////////////////////////////
		// RemoteControl::start();
		// while(1);
		// /////////////////////////////

		//
		// Get SIM CCID to check for its presence
		//
		if(!GSM::sim_card_present())
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("No SIM card detected!"));
			Utils::serial_style(STYLE_RESET);
		}

		//
		// Sync RTC
		//
		if(RTC::sync() != RET_OK)
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

	// Log time sync event
	Log::log(Log::Code::TIME_SYNC);

	// Log boot event after syncing time
	Log::log(Log::Code::BOOT);

	////////////////////////////////////////////////
	// Read log and print
	////////////////////////////////////////////////
	// Create dummy data

	// SensorData::get_store()->clear_all();
	// TestUtils::create_dummy_sensor_data(10);

	
	// CallHome::handle_remote_control();
	// while(1){}

	// TODO: Make all tasks run on boot and rmeove this
	Utils::serial_style(STYLE_MAGENTA);
	Serial.println(F("Reason: Read sensors"));
	Utils::serial_style(STYLE_RESET);
	WaterSensors::log();
	
	Utils::serial_style(STYLE_BLUE);
	Serial.println(F("Reason: Call home"));
	Utils::serial_style(STYLE_RESET);
	CallHome::start();
	Utils::print_separator(F("Setup finished"));
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
			Serial.println(F("The time is: "));
			RTC::print_time();
		}
	}

	return RET_OK;
}

/******************************************************************************
* Main loop
******************************************************************************/
void loop()
{
	Sleep::sleep();


	// Do wake up self test
	if(wakeup_self_test() != RET_OK)
	{
		Utils::serial_style(STYLE_RED);
		Serial.println(F("Wake up self test failed, going back to sleep."));
		Utils::serial_style(STYLE_RESET);
	}

	if(Sleep::wakeup_reason_is(Sleep::REASON_READ_WATER_SENSORS))
	{
		Utils::serial_style(STYLE_MAGENTA);
		Serial.println(F("Reason: Read sensors"));
		Utils::serial_style(STYLE_RESET);
		WaterSensors::log();
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