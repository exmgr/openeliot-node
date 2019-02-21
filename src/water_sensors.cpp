#include "water_sensors.h"
#include "water_quality_sensor.h"
#include "water_level.h"
#include "rtc.h"
#include "utils.h"

namespace WaterSensors
{
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

		WaterQualitySensor::on();

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
					WaterQualitySensor::off();
					WaterQualitySensor::on();
				}

				Utils::serial_style(STYLE_RESET);
			}
			else
				break;

		}while(--tries);

		WaterQualitySensor::off();

		//
		// Read water level
		//
		WaterLevel::on();

		// TODO: X attempts?
		WaterLevel::measure(&data);

		WaterLevel::off();

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