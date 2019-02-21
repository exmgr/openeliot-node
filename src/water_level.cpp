#include "const.h"
#include "app_config.h"
#include "water_level.h"
#include "sensor_data.h"

namespace WaterLevel
{
	/******************************************************************************
	 * Init
	 *****************************************************************************/	
	RetResult init()
	{
		off();

		analogReadResolution(12);
	}

    /******************************************************************************
     * Turn sensor ON
     ******************************************************************************/
    RetResult on()
    {
		Serial.println(F("Water level sensor ON."));
		digitalWrite(PIN_WATER_LEVEL_PWR, 1);
		delay(1000);

        return RET_OK;
    }

    /******************************************************************************
     * Turn sensor OFF
     ******************************************************************************/
    RetResult off()
    {
		Serial.println(F("Water level sensor OFF."));
		digitalWrite(PIN_WATER_LEVEL_PWR, 0);
		delay(100);
        return RET_OK;
    }

	/******************************************************************************
	 * Read water level sensor and populate SensorData entry structure
	 *****************************************************************************/
	RetResult measure(SensorData::Entry *data)
    {
		Serial.println(F("Measuring water level."));

		// If dummy values, ignore because water quality measure will populate dummy
		// values
        if(MEASURE_DUMMY_WATER_LEVEL)
        {
            return RET_OK;
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
		int mv = ((float)3300 / 4096) * level_raw;

		// Apply compensation
		// ADC has an error that is somewhat linear, except at the highest/lowest values
		mv += WATER_LEVEL_ADC_COMPENSATION_MV;

		// Convert to CM
		int cm = mv / WATER_LEVEL_MV_PER_CM;

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