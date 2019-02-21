#include "sensor_data.h"
#include "CRC32.h"
#include "utils.h"

namespace SensorData
{
	/** 
	 * Store for sensor data
	 * Number of entries per file is the same as the number of entries in a request packet.
	 * This way if a request succeeds, a whole file can be deleted, if not the file remains
	 * to be resent at a later time
	 */
    DataStore<SensorData::Entry> store(SENSOR_DATA_PATH, SENSOR_DATA_ENTRIES_PER_SUBMIT_REQ);

    /******************************************************************************
    * Add water sensor data to storage
	* No packet is added without having its CRC updated first
    ******************************************************************************/
    bool add(SensorData::Entry *data)
    {
        return store.add(data);
    }   

    /******************************************************************************
    * Get pointer to store (for use with reader)
    ******************************************************************************/
    DataStore<SensorData::Entry>* get_store()
    {
        return &store;
    }

    /********************************************************************************
	 * Print all data from a water sensor data strcut
	 * @param data Water sensor data structure
	 *******************************************************************************/
	void print(const SensorData::Entry *data)
	{
		Serial.print(F("Timestamp: "));
		Serial.println(data->timestamp);

		Serial.print(F("Temperature: "));
		Serial.println(data->temperature);

		Serial.print(F("Diss. oxygen: "));
		Serial.println(data->dissolved_oxygen);

		Serial.print(F("Conductivity: "));
		Serial.println(data->conductivity);

		Serial.print(F("PH: "));
		Serial.println(data->ph);

		Serial.print(F("Water level: "));
		Serial.println(data->water_level);
	}
}