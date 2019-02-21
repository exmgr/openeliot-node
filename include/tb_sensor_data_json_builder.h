#ifndef TB_SENSOR_DATA_H
#define TB_SENSOR_DATA_H

#include "struct.h"
#include "const.h"
#include "app_config.h"
#include "sensor_data.h"
#include "json_builder_base.h"

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"

/******************************************************************************
* Helper class to build Thingsboard telemetry JSON from sensor data structures
******************************************************************************/
class TbSensorDataJsonBuilder : public JsonBuilderBase<SensorData::Entry, SENSOR_DATA_JSON_DOC_SIZE>
{
public:
	RetResult add(const SensorData::Entry *entry);
};

#endif