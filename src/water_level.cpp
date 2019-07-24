#include "const.h"
#include "app_config.h"
#include "water_level.h"
#include "sensor_data.h"
#include "utils.h"

// TODO: Why WaterQuality-Sensor- while this is just WaterLevel?
namespace WaterLevel
{
	//
	// Private functions
	//
	RetResult measure_pwm(SensorData::Entry *data);
	RetResult measure_analog(SensorData::Entry *data);

	/******************************************************************************
	 * Init
	 *****************************************************************************/	
	RetResult init()
	{
		switch(WATER_LEVEL_INPUT_CHANNEL)
		{
		case WATER_LEVEL_CHANNEL_ANALOG:
			pinMode(PIN_WATER_LEVEL_ANALOG, ANALOG);
			break;
		case WATER_LEVEL_CHANNEL_PWM:
			pinMode(PIN_WATER_LEVEL_PWM, INPUT);
			break;
		}		

		analogReadResolution(12);
	}

	/******************************************************************************
	 * Read water level sensor and populate SensorData entry structure
	 * Use appropriate function depending on the WATER_LEVEL_INPUT_CHANNEL switch
	 *****************************************************************************/
	RetResult measure(SensorData::Entry *data)
	{
		switch(WATER_LEVEL_INPUT_CHANNEL)
		{
		case 1:
			return measure_pwm(data);
			break;
		case 2:
			return measure_analog(data);
			break;
		default:
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Invalid water level sensor channel"));
			Utils::serial_style(STYLE_RESET);

			return RET_ERROR;
		}
	}

	/******************************************************************************
	 * Read water level sensor and populate SensorData entry structure
	 * Use sensors PWM channel
	 *****************************************************************************/
	RetResult measure_pwm(SensorData::Entry *data)
	{
		Serial.println(F("Measuring water level (PWM)"));

        // Return dummy values switch
        if(MEASURE_DUMMY_WATER_LEVEL)
        {
            return measure_dummy(data);
        }

		int meas_left = WATER_LEVEL_MEASUREMENTS_COUNT;
		int failures = 0;
		int level_cum = 0;
		do
		{
			int level = pulseIn(PIN_WATER_LEVEL_PWM, 1, WATER_LEVEL_PWM_TIMEOUT_MS * 1000);

			// Ignore invalid values
			// 0 returned when no pulse before timeout
			// Sensor returns max range when no target detected within rage
			// Since PWM has an offset take into account a small tolerance
			if(level == 0 || level >= (WATER_LEVEL_MAX_RANGE_MM - WATER_LEVEL_PWM_MAX_VAL_TOL))
			{
				if(++failures >= WATER_LEVEL_PWM_FAILED_MEAS_LIMIT)
				{
					return RET_ERROR;
				}
			}

			level_cum += level;
			delay(WATER_LEVEL_DELAY_BETWEEN_MEAS_MS);
		}while(--meas_left > 0);

		level_cum /= WATER_LEVEL_MEASUREMENTS_COUNT;

		// Convert mm to cm
		data->water_level = level_cum / 10;

		return RET_OK;
	}

	/******************************************************************************
	 * Read water level sensor and populate SensorData entry structure
	 * Use sensors ANALOG channel
	 *****************************************************************************/
	RetResult measure_analog(SensorData::Entry *data)
    {
		Serial.println(F("Measuring water level (analog)"));

        // Return dummy values switch
        if(MEASURE_DUMMY_WATER_LEVEL)
        {
            return measure_dummy(data);
        }

		// X measurements, calc average
		int level_raw = 0;
		for(int i = 0; i < WATER_LEVEL_MEASUREMENTS_COUNT; i++)
		{
			level_raw += analogRead(PIN_WATER_LEVEL_ANALOG);
			delay(WATER_LEVEL_DELAY_BETWEEN_MEAS_MS);
		}
		level_raw /= WATER_LEVEL_MEASUREMENTS_COUNT;

		// Convert to mV
		int mv = ((float)3600 / 4096) * level_raw;

		// Convert to CM
		int cm = (mv / WATER_LEVEL_MV_PER_MM) * 10;

		data->water_level = cm;

		return RET_OK;
	}

	/******************************************************************************
     * Fill water quality struct with dummy data
     * Used for debugging only
     * @param data Output structure
     *****************************************************************************/
    RetResult measure_dummy(SensorData::Entry *data)
    {
        Serial.println(F("Returning dummy values"));

		data->water_level = 64;
        data->water_level += (float)random(-400, 400) / 100;

		// Emulate waiting time
        delay(1000);

        return RET_OK;
    }
}