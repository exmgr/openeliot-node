#ifndef LOG_CODES
#define LOG_CODES

namespace Log
{
    enum Code
    {
        //
        // General codes
        // All 0XX codes
        
        // Device boot
        BOOT = 1,

        // Intentional restart
        RESTART = 2,

        // Calling home handling
        CALLING_HOME = 3,

        // Syncing time
        TIME_SYNC = 4,

        // Sensor data submitted
        // Meta1: Total entries submitted
        // Meta2: Number of CRC failures
        SENSOR_DATA_SUBMITTED = 5,

        // Sensor data submission errors
        // Meta 1: Total HTTP requests
        // Meta 2: Failed HTTP requests
        SENSOR_DATA_SUBMISSION_ERRORS = 6,

        // Log data submitted
        // Meta1: Total entries submitted
        // Meta2: Entries that failed CRC check
        LOG_SUBMITTED = 7,

        // CRC error detected on loaded config data
        DEVICE_CONFIG_DATA_CRC_ERRORS = 11,

        // Device going to sleep
        // Logged only for the main sleep event
        SLEEP = 12,

        // Device waking up from sleep
        // Logged only for the main sleep event
        // Meta1: Reason
        WAKEUP = 13,

        // Syncing time with NTP failed
        NTP_TIME_SYNC_FAILED = 14,

        // Battery measurements from fuel gauge
        // Meta1: Voltage
        // Meta2: Percentage
        BATTERY = 15,

        // Internal environmental sensor measurements (1)
        // Meta1: Temperature (float)
        // Meta2: Relative humidity (float)
        INT_ENV_SENSOR1 = 16,

        // Internal environmental sensor measurements (2)
        // Meta1: Pressure (hPa - int)
        // Meta2: Alt (meters - int)
        INT_ENV_SENSOR2 = 17,        

        // Battery measurements from GSM
        // Meta1: Voltage
        // Meta2: Percentage
        BATTERY_GSM = 18,

        // Remote control: Invalid response received (could not deserialize)
        RC_PARSE_FAILED = 19,

        // Remote control: JSON has invalid format
        RC_INVALID_FORMAT = 20,

        // Temperature measurement from RTC
        // Meta1: Temperature
        RTC_TEMPERATURE = 21,

        // Remote control: HTTP request for data failed
        // Meta1: HTTP response code
        RC_REQUEST_FAILED = 22,

        // New remote control data id received, data will be applied
        // Meta1: Data id
        RC_APPLYING_NEW_DATA = 23,

        // Remote control: new water sensors read interval set
        // Meta1: New value
        RC_WATER_SENSORS_READ_INT_SET_SUCCESS = 24,

        // Remote control: Could not set new water sensors read interval value
        // Meta1: Provided value
        RC_WATER_SENSORS_READ_INT_SET_FAILED = 25,

        // Remote control: new calling home interval set
        // Meta1: New value
        RC_CALL_HOME_INT_SET_SUCCESS = 26,

        // Remote control: Could not set new calling home interval value
        // Meta1: Provided value
        RC_CALL_HOME_INT_SET_FAILED = 27,

        // OTA: Requested by remote control data
        OTA_REQUESTED = 28,

        // OTA: Request doesnt contain fw version
        // Meta1: Current fw version
        OTA_NO_FW_VERSION_SPECIFIED = 29,

        // OTA: Provided FW is the same version as the one already running
        // Meta1: Current fw version
        OTA_FW_SAME_VERSION = 30,

        // OTA: FW URL not provided or empty
        OTA_FW_URL_NOT_SET = 31,

        // OTA: MD5 of provided firmware is not specified
        OTA_MD5_NOT_SET = 32,

        // OTA: Url is invalid
        OTA_URL_INVALID = 33,

        // OTA: File get request failed
        // Meta1: HTTP response code
        OTA_FILE_GET_REQ_FAILED = 34,

        // OTA: File get req did not return OK (200)
        // Meta1: HTTP response code
        OTA_FILE_GET_REQ_BAD_RESPONSE = 35,

        // OTA: File get req response empty
        // Meta1: HTTP response code
        OTA_FILE_GET_REQ_RESP_EMPTY = 36,

        // OTA: Update.begin() failed
        // Meta1: Error code returned by getError()
        OTA_UPDATE_BEGIN_FAILED = 37,

        // OTA: Downloading FW file and writing to partition
        // Meta1: File size as specified in Content-length
        OTA_DOWNLOADING_AND_WRITING_FW = 38,

        // OTA: FW writing complete
        // Meta1: Total bytes
        // Meta2: Bytes flashed
        OTA_WRITING_FW_COMPLETE = 39,

        // OTA: Update not finished
        // Meta1: Error code returned by getError()
        OTA_UPDATE_NOT_FINISHED = 40,

        // OTA: Could not validate and finalize update
        // Meta1: Error code returned by getError()
        OTA_COULD_NOT_FINALIZE_UPDATE = 41,

        // OTA: Downloaded and written to partition succesfully.
        OTA_FINISHED = 42,

        // OTA: Self test passed
        OTA_SELF_TEST_PASSED = 43,

        // OTA: self test failed
        OTA_SELF_TEST_FAILED = 44,

        // OTA: FW is rolled back to old version and device rebooted
        OTA_ROLLING_BACK = 45,

        // OTA: Rolling back to previous FW not possible
        OTA_ROLLBACK_NOT_POSSIBLE = 46,

        // File system free space
        // Meta1: Used bytes
        // Meta2: Free bytes
        FS_SPACE = 47,

        // RTC sync took place
        // Time after sync is log time
        // Meta1: System time at the start of sync
        RTC_SYNC = 48,

        //
        // Could not publish tb client attributes
        // Meta1: HTTP response code
        TB_CLIENT_ATTR_PUBLISH_FAILED = 49,

        //
        // Entered sleep charge
        SLEEP_CHARGE = 50,

        //
        // Sleep charge finished
        SLEEP_CHARGE_FINISHED = 51,

        //
        // Could not read water quality sensor
        //
        WATER_QUALITY_MEASURE_FAILED = 52,

        //
        // Could not read water level sensor
        //
        WATER_LEVEL_MEASURE_FAILED = 53,

        //
        // SPIFFS format complete
        // Meta1: Bytes used before format
        SPIFFS_FORMATTED = 54,

        //
        // SPIFFS format failed
        //
        SPIFFS_FORMAT_FAILED = 55,

        //
        // GSM errors
        // All 1XX codes

        //
        // Couldn not connect GPRS
        //
        GSM_NETWORK_DISCOVERY_FAILED = 100,

        //
        // GSM RSSI
        // Meta1: RSSI
        GSM_RSSI = 101,

        //
        // Sim card not detected
        //
        GSM_NO_SIM_CARD = 102,

        //
        // Could not connect to GSM network
        //
        GSM_GPRS_CONNECTION_FAILED = 103,
        
        //
        // GSM init failed
        //
        GSM_INIT_FAILED = 104
    };
}

#endif