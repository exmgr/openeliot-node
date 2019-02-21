#include "json_builder_base.h"
#include "log.h"
#include "sensor_data.h"

/******************************************************************************
 * Default constructor
 * Initialize ArduinoJSON document on construct
 *****************************************************************************/
template <typename TStruct, int TDocSize>
JsonBuilderBase<TStruct, TDocSize>::JsonBuilderBase()
{
    reset();
}

/******************************************************************************
 * Build and write output json to buffer
 *****************************************************************************/
template <typename TStruct, int TDocSize>
RetResult JsonBuilderBase<TStruct, TDocSize>::build(char *buff_out, int buff_size, bool beautify)
{
    int ret = 0;
    	// serializeJsonPretty(_json_doc, buff_out, buff_size);
	if(beautify)
	{
		ret = serializeJsonPretty(_json_doc, buff_out, buff_size);
	}
	else
	{
		ret = serializeJson(_json_doc, buff_out, buff_size);
	}

	return RET_OK;
}

/******************************************************************************
 * Reset object for reuse
 *****************************************************************************/
template <typename TStruct, int TDocSize>
RetResult JsonBuilderBase<TStruct, TDocSize>::reset()
{
	_json_doc.clear();
	_root_array = _json_doc.template to<JsonArray>();
}

/******************************************************************************
 * Serialize beautified and print
 * Used for debugging
 *****************************************************************************/
template <typename TStruct, int TDocSize>
void JsonBuilderBase<TStruct, TDocSize>::print()
{
    // Buff size should cover all cases
	// TODO: Decide buff size
    char buff[2048] = {0};

    build(buff, sizeof(buff), true);

    Serial.println(buff);
    Serial.print(F("Length: "));
    Serial.println(strlen(buff), DEC);
}

template <typename TStruct, int TDocSize>
bool JsonBuilderBase<TStruct, TDocSize>::is_empty()
{
	return _json_doc.size() < 1;
}


// Forward declarations
template class JsonBuilderBase<Log::Entry, LOG_JSON_DOC_SIZE>;
template class JsonBuilderBase<SensorData::Entry, SENSOR_DATA_JSON_DOC_SIZE>;