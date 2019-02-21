#include "tests.h"
#include <Preferences.h>
#include "CRC32.h"
#include "SPIFFS.h"
#include "const.h"
#include "app_config.h"
#include "gsm.h"
#include "flash.h"
#include "rtc.h"
#include "utils.h"
#include "data_store.h"
#include "data_store_reader.h"
#include "sensor_data.h"
#include "sleep.h"
#include "device_config.h"
#include "limits.h"
#include "remote_control.h"
#include "device_config.h"

namespace Tests
{
	/******************************************************************************
	 * Privates
	 ******************************************************************************/
	enum TestResult
	{
		SUCCESS,
		FAILURE,

		// Test didn't run
		IGNORED = -1
	};

	/** Pointers to test functions mapped to their type */
	RetResult (*test_funcs[])() = {
		[RTC_FROM_GSM] = rtc_from_gsm,
		[DATA_STORE] = data_store,
		[WAKEUP_TIMES] = wakeup_times,
		[DEVICE_CONFIG] = device_config
	};

	/** Test names mapped to their type */
	const char *test_names[] = {
		[RTC_FROM_GSM] = "RTC from GSM",
		[DATA_STORE] = "Buffered data store",
		[WAKEUP_TIMES] = "Wake-up times",
		[DEVICE_CONFIG] = "Device configuration store"
	};

	/******************************************************************************
	 * Test specific config
	******************************************************************************/
	
	//
	// Data store
	//
	// Total packets to write
	const int DATA_STORE_ELEMENTS_TO_WRITE = 150;

	// Path of test data store
	const char *DATA_STORE_PATH = "/test";

	// Entries to write in a file before creating a new one
	const char DATA_STORE_ENTRIES_PER_FILE = 12;

	// Size of a single entry in the data store (header + body)
	const int DATA_STORE_ENTRY_SIZE = sizeof(DataStore<SensorData::Entry>::Entry);

	// Size in bytes of a file that doesn't fit any more entries
	const int DATA_STORE_FULL_FILE_SIZE = DATA_STORE_ENTRY_SIZE *  DATA_STORE_ENTRIES_PER_FILE;

	//
	// Wakeup times
	//
	// How many wake up "times" to calculate starting from now
	const int WAKEUP_TIMES_SERIES_LEN = 100;


	/******************************************************************************
	 * Set dummy date in RTC, ask GSM module to update time from NTP and see if
	 * it has changed
	******************************************************************************/
	RetResult rtc_from_gsm()
	{
		// todo: fix unimplemented non trivial designated initializers then remove
		return RET_ERROR;
		//
		// Update RTC
		//
		Serial.println(F("Updating RTC time"));
		RetResult success = RET_ERROR;
		GSM::init();
		if(GSM::on() == RET_OK)
		{
			if(GSM::connect_persist() == RET_OK)
			{
				if(RTC::sync() != RET_OK)
					Serial.println("Could not update time from GSM.");
				else
					success = RET_OK;
			}

			GSM::off();
		}
		if(!success)
		{
			Serial.println(F("Could not update RTC time."));
		}
		else
		{
			Serial.println(F("RTC updated: "));
			RTC::print_time();
		}

		return success;   
	}

	/******************************************************************************
	 * Buffered data store
	 * Write dummy data, read back, check values
	******************************************************************************/
	RetResult data_store()
	{
		//
		// Mount
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Mounting"));
		Utils::serial_style(STYLE_RESET);
		if(!SPIFFS.begin())
		{
			Serial.println(F("# Could not begin SPIFFS."));
			return RET_ERROR;
		}

		//
		// Format
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Formatting"));
		Utils::serial_style(STYLE_RESET);
		if(!SPIFFS.format())
		{
			Serial.println(F("# Format failed."));
			return RET_ERROR;
		}
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Format done"));
		Utils::serial_style(STYLE_RESET);

		//
		// Create data store 
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Creating dummy entries"));
		Utils::serial_style(STYLE_RESET);
		DataStore<SensorData::Entry> store(DATA_STORE_PATH, DATA_STORE_ENTRIES_PER_FILE);
		SensorData::Entry dummy_data_entries[DATA_STORE_ELEMENTS_TO_WRITE] = {0};

		//
		// Build dummy data, add to store and check after every entry if data written is expected
		//

		// Counter of total bytes written
		int expected_bytes_written = 0;

		for(int i = 0; i < DATA_STORE_ELEMENTS_TO_WRITE; i++)
		{
			// Build new entry dummy data
			SensorData::Entry new_entry = {0};
			new_entry.timestamp = i; // This entry will be later searched for by this id
			new_entry.temperature = random(1000000);
			new_entry.dissolved_oxygen = random(1000000);
			new_entry.conductivity = random(1000000);
			new_entry.ph = random(1000000);
			new_entry.water_level = random(1000000);

			// Write to store
			store.add(&new_entry);
			if(store.commit() != RET_OK)
			{
				Serial.println(F("Could not commit data."));
				return RET_ERROR;
			}

			expected_bytes_written += DATA_STORE_ENTRY_SIZE;

			// Add to dummy array to confirm later
			memcpy(&dummy_data_entries[i], &new_entry, sizeof(new_entry));

			// Expected number of files with size DATA_STORE_ENTRY_SIZE * DATA_STORE_ENTRIES_PER_FILE
			int expected_full_files = ((i+1) * DATA_STORE_ENTRY_SIZE) / DATA_STORE_FULL_FILE_SIZE;

			// Other than full files, only one smaller file can exist and it must be of this size.
			// When 0, last file is full  so there can be more of these which is checked by above
			int expected_smallest_file_size = ((i+1) * DATA_STORE_ENTRY_SIZE) - expected_full_files * DATA_STORE_FULL_FILE_SIZE;
			Serial.print(F("Smallest file must be: "));
			Serial.println(expected_smallest_file_size);
			// Iterate all files and check if above above data is true
			File dir = SPIFFS.open(DATA_STORE_PATH);
			if(!dir)
			{
				Serial.println(F("Could not open data store dir."));
				return RET_ERROR;
			}

			// Number of full files found in store
			int found_full_files = 0;
			// Number of files with size other than that of a full file
			int found_other_size_files = 0;
			// Size of smallest file found. Must be only 1 and of matching size
			int found_smallest_file_size = 0;
			// Bytes written so far
			int found_bytes_written = 0;

			// Iterate all files written so file and collect info
			File f;
			while(f = dir.openNextFile())
			{
				int cur_size = f.size();
				found_bytes_written += cur_size;

				if(cur_size == DATA_STORE_FULL_FILE_SIZE)
					found_full_files++;
				else
				{
					found_other_size_files++;

					// If more than one found, test fails early, no reason to go further
					if(found_other_size_files > 1)
					{
						Serial.println(F("More than 1 non-full files found. Should be only 1. Aborting."));
						return RET_ERROR;
					}

					found_smallest_file_size = f.size();
				}
			}

			// Check results for current run
			if(expected_full_files != found_full_files)
			{
				Serial.println(F("Number of files that reached their max size is not the expected one."));
				Serial.print(F("Expected: "));
				Serial.println(expected_full_files, DEC);
				Serial.print(F("Found: "));
				Serial.println(found_full_files, DEC);
				Serial.println(F("Aborting"));

				Flash::ls();

				return RET_ERROR;
			}
			if(found_smallest_file_size != expected_smallest_file_size)
			{
				Serial.println(F("Smallest file (currently being filled) is not of expected size."));
				Serial.print(F("Expected: "));
				Serial.println(expected_smallest_file_size, DEC);
				Serial.print(F("Found: "));
				Serial.println(found_smallest_file_size, DEC);
				Serial.println(F("Aborting"));

				Flash::ls();

				return RET_ERROR;
			}
			if(expected_bytes_written != found_bytes_written)
			{
				Serial.println(F("Number of written bytes different than expected."));
				Serial.print(F("Expected: "));
				Serial.println(expected_bytes_written, DEC);
				Serial.print(F("Found: "));
				Serial.println(found_bytes_written, DEC);
				Serial.println(F("Aborting"));

				Flash::ls();

				return RET_ERROR;
			}
		}
		Serial.print(F("Finished creating dummy data. Created entries: "));
		Serial.println(DATA_STORE_ELEMENTS_TO_WRITE, DEC);

		Flash::ls();

		//
		// Read data back with read and cross check with dummy_data_entries. Find 
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Reading back"));
		Utils::serial_style(STYLE_RESET);
		DataStoreReader<SensorData::Entry> reader(&store);
		SensorData::Entry *read_back = NULL;

		// Each entry will be searched in dummy_data_entries (by timestamp) and if found it will be marked here
		// At the end this array must be all true
		bool found_dummy_entries[DATA_STORE_ELEMENTS_TO_WRITE] = {false};

		// Entries that failed CRC check
		int failed_crc_entries = 0;

		while(reader.next_file())
		{
			while(read_back = reader.next_entry())
			{
				// Check crc
				if(!reader.entry_crc_valid())
				{
					Serial.println(F("Invalid CRC. Entry: "));
					SensorData::print(read_back);
					failed_crc_entries++;
				}

				// Find entry in dummy_data_entries, check if same and mark as found
				for(int i = 0; i < DATA_STORE_ELEMENTS_TO_WRITE; i++)
				{
					// Find by timestamp (which is actually just an incrementing number)
					if(dummy_data_entries[i].timestamp == read_back->timestamp)
					{				
						// Check if identical (entry by entry in case padding is enabled)
						if(dummy_data_entries[i].conductivity == read_back->conductivity &&
							dummy_data_entries[i].dissolved_oxygen == read_back->dissolved_oxygen &&
							dummy_data_entries[i].ph == read_back->ph &&
							dummy_data_entries[i].temperature == read_back->temperature &&
							dummy_data_entries[i].water_level == read_back->water_level)
						{
							// Mark as found
							found_dummy_entries[i] = true;
							break;
						}
						else
						{
							Serial.println(F("Found stored entry, but data different. Found: "));
							SensorData::print(read_back);
							Serial.println(F("Expected: "));
							SensorData::print(&dummy_data_entries[i]);

							return RET_ERROR;
						}
					}
				}
			}
		}

		// Any entries failing CRC?
		if(failed_crc_entries > 0)
		{
			Utils::serial_style(STYLE_RED);
			Serial.print(F("Entries failed CRC check: "));
			Serial.println(failed_crc_entries, DEC);
			Utils::serial_style(STYLE_RESET);
			return RET_ERROR;
		}

		// Finished reading back all files, check all entries found
		int not_found_count = 0;
		for(int i = 0; i < DATA_STORE_ELEMENTS_TO_WRITE; i++)
		{
			if(!found_dummy_entries[i])
			{
				not_found_count++;
			}
		}

		if(not_found_count > 0)
		{
			Utils::serial_style(STYLE_RED);
			Serial.print(F("Not all stored entries found when reading back. Not found entries: "));
			Serial.println(not_found_count, DEC);
			Utils::serial_style(STYLE_RESET);
			return RET_ERROR;
		}

		//
		// Clean up
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Formatting for clean up."));
		Utils::serial_style(STYLE_RESET);
		if(!SPIFFS.format())
		{
			Serial.println(F("# Format failed."));
			return RET_ERROR;
		}

		Serial.println(F("Done!"));
		Flash::ls();

		return RET_OK;
	}

	/******************************************************************************
	 * Sleep
	 * Use dummy wakeup schedule and check if calc_next_wakeup returns correct vals
	 ******************************************************************************/
	RetResult wakeup_times()
	{
		const Sleep::WakeupScheduleEntry ref_schedule[] = 
		{
			{ Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 2},
			{ Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 5},
			{ Sleep::WakeupReason::REASON_CALL_HOME, 7},
			{ Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 10},
			{ Sleep::WakeupReason::REASON_CALL_HOME, 40},
			{ Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 42},
			{ Sleep::WakeupReason::REASON_CALL_HOME, 84},
		};
		const int ref_schedule_len = sizeof(ref_schedule) / sizeof(ref_schedule[0]);

		int ref_times[WAKEUP_TIMES_SERIES_LEN] = {0};
		int t = 0;

		for(int i = 0; i < WAKEUP_TIMES_SERIES_LEN; i++)
		{
			// Check which is closer
			for(int j = 0; j < ref_schedule_len; j++)
			{

			}
		}

		// TODO: Not ready


		return RET_OK;
	}

	/******************************************************************************
	* Configuration store
	******************************************************************************/
	RetResult device_config()
	{
		Utils::serial_style(STYLE_RED);
		Serial.println(F("This test will delete existing device config."));
		Utils::serial_style(STYLE_RESET);
		//
		// Erase current config entry in NVS
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Resetting"));
		Utils::serial_style(STYLE_RESET);
		Preferences prefs;
		if(!prefs.begin(DEVICE_CONFIG_NVS_NAMESPACE_NAME))
		{
			Serial.println(F("Could not begin NVS."));
			return RET_ERROR;
		}

		if(!prefs.remove(DEVICE_CONFIG_NVS_NAMESPACE_NAME))
		{
			Serial.println(F("Could not remove existing NVS key, probably doesn't exist."));
		}
		else
		{
			Serial.println(F("Removed existing key."));
		}
		prefs.end();

		//
		// Init config store
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Initializing config store"));
		Utils::serial_style(STYLE_RESET);
		DeviceConfig::init();

		//
		// Write dummy data to store
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Writing dummy data to store."));
		Utils::serial_style(STYLE_RESET);

		// Create dummy vals
		RemoteControl::Data dummy_remote_control_data(random(INT_MAX), random(INT_MAX),
		random(INT_MAX) % 2, random(INT_MAX) % 2);

		DeviceConfig::Data dummy_config;
		dummy_config.clean_reboot = random(INT_MAX) % 2;
		dummy_config.apply_remote_control_on_boot = random(INT_MAX) % 2;
		dummy_config.remote_control_data = dummy_remote_control_data;


		// Do not include crc32 field in the calculation
		dummy_config.crc32 = 0;
		dummy_config.crc32 = Utils::crc32((uint8_t*)&dummy_config, sizeof(dummy_config));

		// Set
		DeviceConfig::set_clean_reboot(dummy_config.clean_reboot);
		DeviceConfig::set_apply_remote_control(dummy_config.apply_remote_control_on_boot);
		DeviceConfig::set_remote_control_data(&dummy_config.remote_control_data);

		// Write
		DeviceConfig::commit();

		DeviceConfig::print(&dummy_config);

		//
		// Read back and verify
		//
		Utils::serial_style(STYLE_BLUE);
		Serial.println(F("# Reading back"));
		Utils::serial_style(STYLE_RESET);

		const DeviceConfig::Data *read_back = DeviceConfig::get();

		// Reading or crc failed)
		if(read_back == nullptr)
		{
			Serial.println(F("Could not read back config."));
			return RET_ERROR;
		}

		DeviceConfig::print(read_back);

		// Clear to finish
		prefs.remove(DEVICE_CONFIG_NVS_NAMESPACE_NAME);

		Serial.println(F("Done!"));

		return RET_OK;
	}

	/******************************************************************************
	 * Run all tests and print report
	******************************************************************************/    
	void run_all()
	{
		int test_count = sizeof(test_funcs) / sizeof(test_funcs[0]);

		// Build array of all tests
		// Test ids are assumed sequential from 0
		TestId tests[test_count];
		for(int i=0; i<test_count; i++)
		{
			tests[i] = (TestId)i;
		}

		run(tests, test_count);
	}

	/******************************************************************************
	* Run tests specified by the test array
	* @param tests Array of tests to run
	* @param count Array size
	******************************************************************************/ 
	void run(TestId tests[], int count)
	{
		if(count < 1)
			return;

		char block_title[] = "Running XXX tests";
		snprintf(block_title, sizeof(block_title), "Running %d tests", count);
		Utils::print_block(F(block_title));
		Serial.println();

		const int test_count = sizeof(test_funcs) / sizeof(test_funcs[0]);

		// Test results
		// Make space for all tests, even those that arent going to be ran because
		// array index is test id
		TestResult results[test_count];
		memset(&results, IGNORED, sizeof(results));
		char name[100] = "";
		
		for(int i=0; i<count; i++)
		{
			TestId test_id = tests[i];

			Serial.println();
			snprintf(name, sizeof(name), "%s (%d/%d)", test_names[test_id], i+1, count);
			Utils::print_separator(F(name));
			Serial.println();

			if(test_funcs[test_id]() == RET_OK)
			{
				results[test_id] = SUCCESS;
			}
			else
			{
				results[test_id] = FAILURE;
			}
		}

		// Print results
		Serial.println();
		Utils::print_separator(F("Results"));
		Serial.println();

		bool overall_success = true;
		for(int i = 0; i < test_count; i++)
		{
			switch(results[i])
			{
			case SUCCESS:
				Utils::serial_style(STYLE_GREEN);
				Serial.print(F("[SUCCESS] "));
				Utils::serial_style(STYLE_RESET);
				break;
			case FAILURE:
				Utils::serial_style(STYLE_RED);
				Serial.print(F("[FAILURE] "));
				Utils::serial_style(STYLE_RESET);

				overall_success = false;
				break;
			case IGNORED:
				Utils::serial_style(STYLE_YELLOW);
				Serial.print(F("[IGNORED] "));
				Utils::serial_style(STYLE_RESET);
				break;		
			default:
				Serial.println(F("Invalid test result value."));
			}

			// Print name
			Serial.println(test_names[i]);
		}

		if(overall_success)
		{
			Utils::serial_style(STYLE_GREEN);
			Serial.println(F("All tests passed!"));
			Utils::serial_style(STYLE_RESET);
		}
		else
		{
			Utils::serial_style(STYLE_RED);
			Serial.println(F("Tests failed."));
			Utils::serial_style(STYLE_RESET);
		}
	}
} // Tests
