#include "test_utils.h"
#include "log.h"
#include "data_store_reader.h"
#include "utils.h"
#include "sensor_data.h"
#include "esp_partition.h"

/******************************************************************************
* Various functions to help with debugging
******************************************************************************/
namespace TestUtils
{
	/******************************************************************************
	* Reads and prints all log entries
	******************************************************************************/
	void print_all_logs()
	{
		DataStoreReader<Log::Entry> reader(Log::get_store());

		const Log::Entry *entry = NULL;

		while(entry = reader.next_entry()) // TODO: New implementation
		{
			Log::print(entry);
			Utils::print_separator(NULL);
		}
	}

	/******************************************************************************
	* Create dummy sensor data
	******************************************************************************/
	void create_dummy_sensor_data(int count)
	{
		SensorData::Entry data = {0};

		for(int i = 0; i < count; i++)
		{
			data.timestamp = time(NULL);
			data.temperature = i;
			data.dissolved_oxygen = i;
			data.conductivity = i;
			data.ph = i;
			data.water_level = i;

			SensorData::add(&data);
		}
	}

	/******************************************************************************
	 * Print partitions
	 *****************************************************************************/
	void print_partition_info()
	{
		esp_partition_type_t type;
		esp_partition_iterator_t iterator;
		const esp_partition_t *partition = NULL;

		Utils::serial_style(STYLE_BLUE);
		Serial.printf("%-10s %-10s %-10s %-10s %-10s %-10s\n", "Label", "Type", "Subtype", "Addr", "Size", "Encrypted");
		Serial.printf("%.*s\n", 65, "======================================================================================");
		Utils::serial_style(STYLE_RESET);

		iterator = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
		while(iterator != NULL)
		{
			partition = esp_partition_get(iterator);	

			// Decide partition subtype
			Serial.printf("%-10s %-10s %-10d %-10d %-10d %-10s\n",
					partition->label, "App", partition->subtype,
					partition->address, partition->size, partition->encrypted ? "Yes" : "No");

			iterator = esp_partition_next(iterator);
		}
		iterator = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
		while(iterator != NULL)
		{
			partition = esp_partition_get(iterator);	

			// Decide partition subtype
			Serial.printf("%-10s %-10s %-10d %-10d %-10d %-10s\n",
					partition->label, "Data", partition->subtype,
					partition->address, partition->size, partition->encrypted ? "Yes" : "No");

			iterator = esp_partition_next(iterator);
		}

		esp_partition_iterator_release(iterator);
	}

	/******************************************************************************
	* Print FReeRTOS task size
	******************************************************************************/
	void print_stack_size()
	{
		Serial.print(F("Stack size: "));
		Serial.println(uxTaskGetStackHighWaterMark(NULL), DEC);
	}

	/******************************************************************************
	 * Malloc in steps until memory is full and print results
	 * Independent test, not very useful, used only during debugging
	 ******************************************************************************/
	void fill_heap()
	{	
		uint8_t *p = NULL;
		const int step = 1024;
		const int bytes = 300000;

		for(int i=0; i < bytes; i += step)
		{
			Serial.print(F("Allocating "));
			Serial.print(i / 1024);
			Serial.println(F(" KB"));

			// p = (uint8_t*)malloc(i);
			p = (uint8_t*)malloc(step);
			if(p == NULL)
			{
				Serial.println(F("FAILED"));
				break;
			}
			// free(p);
		}
	}
}