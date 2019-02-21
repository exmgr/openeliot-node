#include "SPIFFS.h"
#include "log.h"
#include "const.h"
#include "rtc.h"

namespace Log
{
	/** Log data store */
	DataStore<Log::Entry> store(LOG_DATA_PATH, SENSOR_DATA_ENTRIES_PER_SUBMIT_REQ);

	/******************************************************************************
	* Create log entry with current timestamp.
	* @param code Error code
	* @param meta1 Metadata field 1
	* @param meta2 Metadata field 2
	******************************************************************************/
	bool log(Log::Code code, uint32_t meta1, uint32_t meta2)
	{
		Entry entry;

		entry.code = code;
		entry.meta1 = meta1;
		entry.meta2 = meta2;

		entry.timestamp = RTC::get_timestamp();

		store.add(&entry);
		store.commit();

		// Serial.println(F("Log:"));
		// print(&entry);

		return RET_OK;
	}

	/******************************************************************************
	* Get pointer to store (for use with reader)
	******************************************************************************/
	DataStore<Entry>* get_store()
	{
		return &store;
	}

	/******************************************************************************
	* Commit log data store to flash
	******************************************************************************/
	RetResult commit()
	{
		return store.commit();
	}

	/********************************************************************************
	 * Print all data from a log entry struct
	 * @param entry Pointer to log entry struct
	 *******************************************************************************/
	void print(const Log::Entry *entry)
	{
		Serial.print(F("Code: "));
		Serial.println(entry->code);

		Serial.print(F("Meta 1: "));
		Serial.println(entry->meta1);

		Serial.print(F("Meta 2: "));
		Serial.println(entry->meta2);

		Serial.print(F("Timestamp: "));
		Serial.print(entry->timestamp);
		Serial.print(" (");
		Serial.print(ctime((time_t*)&entry->timestamp));
		Serial.println(" )");
	}
}