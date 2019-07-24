/******************************************************************************
 * Water Quality sensor related functions.
 * Sensor: In Situ AquaTROLL 400
 ******************************************************************************/
#include "water_quality_sensor.h"
#include "utils.h"
#include "rtc.h"
#include "sdi12_adapter.h"

namespace WaterQualitySensor
{
    /******************************************************************************
     * Initialization
     ******************************************************************************/
    RetResult init()
    {
        return RET_OK;
    }

    /******************************************************************************
     * Send measure command to the sensor and fill data structure
     * Aquatroll is configured to return 5 measurements (in this order):
     * RDO - Dissolved oxygen (concentration) - mg/L
     * RDO - Dissolved oxygen (%saturation) - %Sat
     * RDO - Temperature - C
     * Cond - Specific Conductivity - uS/cm
     * pH/ORP
     * @param data Output structure
     ******************************************************************************/
    RetResult measure(SensorData::Entry *data)
    {
        Serial.println("Measuring water quality.");

        // Return dummy values switch
        if(MEASURE_DUMMY_WATER_QUALITY)
        {
            return measure_dummy(data);
        }

        // Zero structure
        memset(data, 0, sizeof(SensorData::Entry));

        // I2C slave SDI12 adapter
        SDI12Adapter adapter(SDI12_I2C_ADDRESS);
        adapter.begin(PIN_SDI12_I2C1_SDA, PIN_SDI12_I2C1_SCL);

        // Pointer to response buffer
        const char *response = adapter.get_data();

        // Address parsed from response
        // All responses contain the SDI12 device address as the first char
        int addr = 0;
        // Number of vals parsed from response
        int vals = 0;

        // Start adapter
        adapter.begin(PIN_I2C1_SDA, PIN_I2C1_SCL);

        // Request measurement start
        adapter.send("0MC!");
        delay(SDI12_COMMAND_WAIT_TIME_MS);
        adapter.read();

        // Parse response.
        // Response format must be ABBBC
        // BBB: seconds to wait
        // C: number of measurements to be returned
        int secs_to_wait = 0, measured_values = 0;

        vals = sscanf(response, "%1d%3d%1d", &addr, &secs_to_wait, &measured_values);

        // Must be exactly 3 vals
        if(vals != 3)
        {
            Serial.println(F("Invalid response returned."));
            return RET_ERROR;
        }

        // Validate vals
        // Check address
        if(addr != 0)
        {
            Serial.print("Invalid address value, expected: ");
            Serial.println(addr);
            return RET_ERROR;
        }
        // Check if seconds within range. Range values are chosen empirically.
        // If values not within range, something is wrong with the response or with the
        // configuration of Aquatroll
        if(secs_to_wait > WATER_QUALITY_MEASURE_WAIT_SEC_MAX)
        {
            Serial.println(F("Invalid number of seconds to wait for measurements."));
            return RET_ERROR;
        }
        // The exact number of measured values is known and configured into Aquatroll
        if(measured_values != WATER_QUALITY_NUMBER_OF_MEASUREMENTS)
        {
            Serial.print(F("Invalid number of measured values. Expected: "));
            Serial.println(WATER_QUALITY_NUMBER_OF_MEASUREMENTS, DEC);
            return RET_ERROR;
        }


        // TODO: 
        // When sleeping, pins go low and sensor is disabled.
        // Use RTC GPIO so power pin  can stay high and ESP32 can sleep instead of waiting
        Serial.print(F("Waiting (sec): "));
        Serial.println(secs_to_wait + WATER_QUALITY_MEASURE_EXTRA_WAIT_SECS);
        delay((secs_to_wait + WATER_QUALITY_MEASURE_EXTRA_WAIT_SECS) * 1000);

        /*
        // Sleep requested number of seconds with an added tolerance
        Serial.print(F("Sleeping for seconds: "));
        Serial.println(secs_to_wait, DEC);
        Serial.flush();
        esp_sleep_enable_timer_wakeup((secs_to_wait + WATER_QUALITY_MEASURE_EXTRA_WAIT_SECS)  * 1000000);
        esp_light_sleep_start();

        Serial.println(F("Wake up!"));
        */
        

        //
        // Request measurement data
        // Measurement data is return in batches of 3 (protocol limit)
        // Data format is A+N1+N2+N3CRC. CRC is ignored, it is parsed by adapter's
        // crc calc function.
        const char *parse_format = "%1d+%f+%f+%f";
        SensorData::Entry sensor_data = {0};

        // Vars to receive all the measurement data
        // Data will be copied to output structure only after successfull measurement
        float d1 = 0, d2 = 0, d3 = 0, d4 = 0, d5 = 0;
        
        //
        // Request and parse 1st batch (3 vars)
        //
        adapter.send("0D0!");
        delay(SDI12_COMMAND_WAIT_TIME_MS);
        adapter.read();

        if(!adapter.check_crc())
        {
            Serial.println("Response failed CRC check.");
            return RET_ERROR;
        }
        vals = sscanf(response, parse_format, &addr, &d1, &d2, &d3);
        if(addr != 0 || vals != 4)
        {
            Serial.println("Could not parse response.");
            return RET_ERROR;
        }

        //
        // Request and parse 2nd batch (3 vars)
        //
        adapter.send("0D1!");
        delay(SDI12_COMMAND_WAIT_TIME_MS);
        adapter.read();

        if(!adapter.check_crc())
        {
            Serial.println("Response failed CRC check.");
            return RET_ERROR;
        }
        vals = sscanf(response, parse_format, &addr, &d4, &d5);
        if(addr != 0 || vals != 3)
        {
            Serial.println("Could not parse response.");
            return RET_ERROR;
        }

        //
        // All data received successfully, fill strcture
        //
        data->dissolved_oxygen = d1;
        data->temperature = d3;
        data->conductivity = d4;
        data->ph = d5;

        Utils::serial_style(STYLE_GREEN);
        Serial.println(F("All water quality data is received successfully."));
        Utils::serial_style(STYLE_RESET);

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
        static SensorData::Entry dummy = {
            24, 20, 50, 7, 200, 64
        };

        // Add/subtract random offset
        dummy.conductivity += (float)random(-400, 400) / 100;
        dummy.dissolved_oxygen += (float)random(-400, 400) / 100;
        dummy.ph += (float)random(-200, 200) / 100;
        dummy.temperature += (float)random(-400, 400) / 100;

        memcpy(data, &dummy, sizeof(dummy));

        // Emulate waiting time
        delay(1000);

        return RET_OK;
    }
}