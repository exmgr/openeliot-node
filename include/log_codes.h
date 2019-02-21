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
        // Meta 1: Failed submissions (records prepare for sumbission but submission failed)
        // Meta 2: Records failed CRC
        SENSOR_DATA_SUBMISSION_ERRORS = 6,

        // Log data submitted
        // Meta1: Total entries submitted
        // Meta2: Entries that failed CRC check
        LOG_SUBMITTED = 7,

        // CRC error detected on loaded sensor data
        SENSOR_DATA_CRC_ERRORS = 9,

        // CRC error detected on loaded log data
        LOG_DATA_CRC_ERRORS = 10,

        // CRC error detected on loaded config data
        CONFIG_DATA_CRC_ERRORS = 11,

        // Device going to sleep
        // Logged only for the main sleep event
        SLEEP = 12,

        // Device waking up from sleep
        // Logged only for the main sleep event
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

        // Remote control, invalid response received (could not deserialize)
        REMOTE_CONTROL_PARSE_FAILED = 19,

        // Remote control, JSON has invalid format
        REMOTE_CONTROL_INVALID_FORMAT = 20,

        //
        // GSM errors
        // All 1XX codes

        // Connection to network failed
        GSM_CONNECT_FAILED = 100,

        //
        // GSM RSSI
        // Meta1: RSSI
        GSM_RSSI = 101
    };
}

#endif