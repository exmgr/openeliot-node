#include "water_sensor_data.h"
#include "CRC32.h"
#include "utils.h"
#include "common.h"

namespace WaterSensorData
{
	/** 
	 * Store for water sensor data
	 * Number of entries per file is the same as the number of entries in a request packet.
	 * This way if a request succeeds, a whole file can be deleted, if not the file remains
	 * to be resent at a later time
	 */
    DataStore<WaterSensorData::Entry> store(WATER_SENSOR_DATA_PATH, WATER_SENSOR_DATA_ENTRIES_PER_SUBMIT_REQ);

    /******************************************************************************
    * Add water sensor data to storage
	* No packet is added without having its CRC updated first
    ******************************************************************************/
    RetResult add(WaterSensorData::Entry *data)
    {
		RetResult ret = store.add(data);

		// Commit on every add
		store.commit();

        return ret;
    }   

    /******************************************************************************
    * Get pointer to store (for use with reader)
    ******************************************************************************/
    DataStore<WaterSensorData::Entry>* get_store()
    {
        return &store;
    }

    /********************************************************************************
	 * Print all data from a water sensor data strcut
	 * @param data Water sensor data structure
	 *******************************************************************************/
	void print(const WaterSensorData::Entry *data)
	{
		debug_print(F("Timestamp: "));
		debug_println(data->timestamp);

		debug_print(F("Temperature: "));
		debug_println(data->temperature);

		debug_print(F("Diss. oxygen: "));
		debug_println(data->dissolved_oxygen);

		debug_print(F("Conductivity: "));
		debug_println(data->conductivity);

		debug_print(F("PH: "));
		debug_println(data->ph);

		debug_print(F("ORP: "));
		debug_println(data->orp);

		debug_print(F("Pressure: "));
		debug_println(data->pressure);

		debug_print(F("Depth (cm): "));
		debug_println(data->depth_cm);

		debug_print(F("Depth (ft): "));
		debug_println(data->depth_ft);

		debug_print(F("Water level: "));
		debug_println(data->water_level);

		debug_print(F("Water Presence: "));
		debug_println(data->presence);
	}

	/******************************************************************************
	* Print file in a compact horizontal format
	******************************************************************************/
	void print_array(Entry *entries, int size)
	{
		int rows = size / sizeof(Entry);

		debug_print_i(F("| Rows: "));
		debug_print_w(rows)
		debug_println_i(F(" |"));
		debug_printf_i("| %-3s | ", "#");
		debug_printf_i("%-10s | ", "ts");
		debug_printf_i("%-10s | ", "Temp.");
		debug_printf_i("%-10s | ", "Diss.");
		debug_printf_i("%-10s | ", "Cond.");
		debug_printf_i("%-10s | ", "PH");
		debug_printf_i("%-10s | ", "ORP");
		debug_printf_i("%-10s | ", "Press.");
		debug_printf_i("%-10s | ", "Dep-CM");
		debug_printf_i("%-10s | ", "Dep-FT");
		debug_printf_i("%-10s | ", "TSS");
		debug_printf_i("%-10s | ", "W.Pres");
		debug_printf_i("%-10s | ", "W.Lvl");
		debug_println();

		for (size_t i = 0; i < rows; i++)
		{
			Entry *entry = &entries[i];

			debug_printf_i("| %3d |", i);
			debug_printf_i(" %10d |", entry->timestamp);
			debug_printf_i(" %10.2f |", entry->temperature);
			debug_printf_i(" %10.2f |", entry->dissolved_oxygen);
			debug_printf_i(" %10.2f |", entry->conductivity);
			debug_printf_i(" %10.2f |", entry->ph);
			debug_printf_i(" %10.2f |", entry->orp);
			debug_printf_i(" %10.2f |", entry->pressure);
			debug_printf_i(" %10.2f |", entry->depth_cm);
			debug_printf_i(" %10.2f |", entry->depth_ft);
			debug_printf_i(" %10.2f |", entry->tss);
			debug_printf_i(" %10d |", entry->presence);
			debug_printf_i(" %10.2f |", entry->water_level);
			debug_println();
		}
	}

	/******************************************************************************
	* Print a data file
	******************************************************************************/
	void print_file(char *name)
	{
		// char filename[30] = "";
		// snprintf(filename, sizeof(filename), "/was/%s", name);

		debug_print_i(F("Printing file: "));
		debug_println(name);

		File f = SPIFFS.open(name, "r");
		if(!f)
		{
			debug_println_e(F("Could not open file for reading."));
			return;
		}

		debug_print(F("Size: "));
		debug_println(f.size(), DEC);

		Entry entries[20] = {0};
		int cur_entry = 0;
		int total_bytes_read = 0;

		while(total_bytes_read < f.size())
		{
			Entry entry = {0};
			int bytes_read = f.readBytes((char*)&entry, sizeof(Entry));
			debug_print(F("Cur entry: "));
			debug_println(cur_entry, DEC);

			total_bytes_read += bytes_read;

			if(bytes_read != sizeof(Entry))
			{
				debug_print(F("Invalid size read: "));
				debug_println(bytes_read, DEC);
			}
			else
			{
				memcpy((void*)&entries[cur_entry], &entry, sizeof(Entry));
				cur_entry++;
			}
		}

		print_array(entries, cur_entry * sizeof(Entry));
				

		f.close();
	}
}