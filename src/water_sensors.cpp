#include "water_sensors.h"
#include "water_quality_sensor.h"
#include "water_level.h"
#include "rtc.h"
#include "utils.h"
#include "log.h"

namespace WaterSensors
{
	/******************************************************************************
	 * Init
	 *****************************************************************************/
	RetResult init()
	{
		// Set up pins
		pinMode(PIN_WATER_SENSORS_PWR, OUTPUT);

		// Make sure sensors are off
		off();
	}

	/******************************************************************************
	 * Turn water sensors ON
	 * Both water sensors' power is controlled from the same GPIO
	 *****************************************************************************/
	RetResult on()
	{
		Serial.println(F("Water sensors ON."));

		#ifdef TCALL_H
			// Turn IP5306 power boost OFF to reduce idle current
			Utils::ip5306_set_power_boost_state(true);
		#endif

		digitalWrite(PIN_WATER_SENSORS_PWR, 1);
		delay(1000);

        return RET_OK;
	}

	/******************************************************************************
	 * Turn water sensors OFF
	 *****************************************************************************/
	RetResult off()
	{
		Serial.println(F("Water sensors OFF."));

		digitalWrite(PIN_WATER_SENSORS_PWR, 0);
		delay(100);

		#ifdef TCALL_H
			// Turn IP5306 power boost OFF to reduce idle current
			Utils::ip5306_set_power_boost_state(false);
		#endif

        return RET_OK;
	}

	/******************************************************************************
	* Read all water sensors and log their data in memory
	******************************************************************************/
	void log()
	{
		SensorData::Entry data = {0};

		//
		// Read water quality sensor
		// Try reading X times. If fails, cycle power and try once more.
		RetResult ret = RET_ERROR;
		int tries = 3;

		WaterSensors::on();

		do
		{
			ret = WaterQualitySensor::measure(&data);

			if(ret != RET_OK)
			{
				Utils::serial_style(STYLE_RED);
				Serial.print(F("Failed to read water quality sensor."));

				if(tries > 1)
				{
					Serial.print(F(" Retrying..."));
					delay(WATER_QUALITY_RETRY_WAIT_MS);
				}
				Serial.println();

				// Next try is last, cycle power
				if(tries == 2)
				{
					Serial.println(F("Cycling sensor power."));	
					WaterSensors::off();
					WaterSensors::on();
				}

				Utils::serial_style(STYLE_RESET);
			}
			else
				break;

		}while(--tries);

        // Log error
		if(ret == RET_ERROR)
		{
			Log::log(Log::WATER_QUALITY_MEASURE_FAILED);
		}

		//
		// Read water level
		//
		if(WaterLevel::measure(&data) != RET_OK)
		{
			Log::log(Log::WATER_LEVEL_MEASURE_FAILED);
		}

		WaterSensors::off();

		//
		// Set timestamp
		//
		data.timestamp = RTC::get_timestamp();

		Serial.println(F("Water sensor data:"));
		SensorData::print(&data);

		SensorData::add(&data);
		SensorData::get_store()->commit();
	}

	/******************************************************************************
	* Check if measure interval (mins) value is within valid range
	******************************************************************************/
	bool is_measure_int_value_valid(int interval)
	{
		return (interval >= MEASURE_WATER_SENSORS_INT_MINS_MIN && interval <= MEASURE_WATER_SENSORS_INT_MINS_MAX);
	}
}