#include "log_json_builder.h"

RetResult LogJsonBuilder::add(const Log::Entry *entry)
{
    JsonArray json_entry = _root_array.createNestedArray();

    if(json_entry.add(entry->timestamp) == false ||
        json_entry.add((int)entry->code) == false ||
        json_entry.add(entry->meta1) == false ||
        json_entry.add(entry->meta2) == false)
    {
        Serial.println(F("Could not add log entries to JSON."));
        return RET_ERROR;
    }

	return RET_OK;
}