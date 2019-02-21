#include "tb_sensor_data_json_builder.h"
#include "utils.h"

/******************************************************************************
 * Add packet to request
 *****************************************************************************/
RetResult TbSensorDataJsonBuilder::add(const SensorData::Entry *entry)
{
	JsonObject json_entry = _root_array.createNestedObject();

	json_entry["ts"] = (long long)entry->timestamp * 1000;
	JsonObject values = json_entry.createNestedObject("values");
	
	values["s_do"] = entry->dissolved_oxygen;
	values["s_temp"] = entry->temperature;
	values["s_ph"] = entry->ph;
	values["s_cond"] = entry->conductivity;
	
	// Check the last one, no need to check all, if last doesnt fit into
	// the json doc, doc is full already
	if((values["s_wl"] = entry->water_level) == false)
	{
		Serial.println(F("Could not add sensor data to JSON."));
		return RET_ERROR;
	}

	return RET_OK;
} 