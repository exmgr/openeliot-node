#include "int_env_sensor.h"
#include "const.h"
#include "struct.h"
#include "SparkFunBME280.h"
#include "utils.h"
#include "log.h"

namespace IntEnvSensor
{
	//
	// Private vars
	//
	/** Sensor object */
	BME280 sensor;

	/******************************************************************************
	* Init fuel gauge
	******************************************************************************/
	RetResult init()
	{
		sensor.setI2CAddress(BME280_I2C_ADDR);

		if(sensor.beginI2C(Wire) == false)
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Could not communicate with BME280."));
			Utils::serial_style(STYLE_RESET);
			return RET_ERROR;
		}
	}

	/******************************************************************************
	* Wake up, measure and go back to sleep
	* Pass NULL to ignore a measurement
	* @param temp Temperature output
	* @param hum HUmidity output
	* @param alt Altitude output
	* @param press Pressure output
	******************************************************************************/
	RetResult read(float *temp = NULL, float *hum = NULL, int *press = NULL, int *alt = NULL)
	{
		// Wake up
		sensor.setMode(01);

		if(temp != NULL)
		{
			*temp = sensor.readTempC();
		}

		if(hum != NULL)
		{
			*hum = sensor.readFloatHumidity();
		}

		if(press != NULL)
		{
			*press = (int)(sensor.readFloatPressure() / 100);
		}

		if(alt != NULL)
		{
			*alt = (int)sensor.readFloatAltitudeMeters();
		}

		// Go back to sleep
		sensor.setMode(00);

		return RET_OK;
	}

	/******************************************************************************
    * Store measurements in log
    *****************************************************************************/
    RetResult log()
    {
        float temp = 0, hum = 0;
		int press = 0, alt = 0;

        if(read(&temp, &hum, &press, &alt) == RET_ERROR)
        {
            return RET_ERROR;
        }
		
		Log::log(Log::INT_ENV_SENSOR1, temp, hum);
		Log::log(Log::INT_ENV_SENSOR2, press, alt);

		Utils::serial_style(STYLE_BLUE);
		Serial.printf("Temp: %3.2fC | Hum: %3.2f%% | Press: %dhPa | Alt: %dm\n",
			temp, hum, press, alt);
		Utils::serial_style(STYLE_RESET);

        return RET_OK;
    }


} // namespace IntEnvSensor