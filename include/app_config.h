#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include "struct.h"
#include "Arduino.h"
#include "sleep.h"


/******************************************************************************
 * General
 *****************************************************************************/

/** Enable NBIoT mode. If false GSM is used */
const bool NBIOT_MODE = false;

/** When sleeping treat minutes as seconds. Used for debugging */
const bool SLEEP_MINS_AS_SECS = true;

/** Return dummy values when measuring water quality */
const bool MEASURE_DUMMY_WATER_QUALITY = false;

/** Return dummy values when measuring water level */
const bool MEASURE_DUMMY_WATER_LEVEL = false;

/** Use external RTC when syncing time */
const bool USE_EXTERNAL_RTC = true;

/** Display debug message in serial output. If disabled no text will be output. */
// #define SERIAL_DEBUG_MSGS false;

/**
 * Device descriptor array.
 * Contains all devices in use and device-specific configuration for each.
 * Devices not on this list will not boot.
 * Params:
 *  - Device id
 *  - Mac address (devices are mapped to this list by this)
 *  - GPRS APN
 *  - Thingsboard device token
 */
const DeviceDescriptor DEVICE_DESCRIPTORS[] =
{
    {1, "00:00:00:00:00:00", "your_apn", "thingsboard_token"}, // Feather
};

/** From this level and below battery level is considered critical.
 * Battery level affects settings used (eg. measuring intervals)
*/
const int BATTERY_LEVEL_CRITICAL = 40;

/** Size of DataStore's buffer that holds uncommited data. Once this buffer
 is full, data is commited to flash */
const int DATA_STORE_BUFFER_ELEMENTS = 10;

/** NTP server used by GSM module for time sync */
const char NTP_SERVER[] PROGMEM = "pool.ntp.org";
// const char NTP_SERVER[] PROGMEM = "ntp.grnet.gr";
// const char NTP_SERVER[] PROGMEM = "gr.pool.ntp.org"; //returns local time (?)

/******************************************************************************
* Sleep/wake up schedules
******************************************************************************/
/** Default schedule used when battery levels normal and custom settings have
 * not been set by user yet
 */
const Sleep::WakeupScheduleEntry WAKEUP_SCHEDULE_DEFAULT[] =
{
    {Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 5},
    {Sleep::WakeupReason::REASON_CALL_HOME, 25}
};

/** Size of wake up schedule. Size must be known so it can be written to DeviceConfig */
const int WAKEUP_SCHEDULE_LEN = sizeof(WAKEUP_SCHEDULE_DEFAULT) / sizeof(WAKEUP_SCHEDULE_DEFAULT[0]);

/** Used when battery levels critical (user/default settings ignored)  */
const Sleep::WakeupScheduleEntry WAKEUP_SCHEDULE_BATT_CRITICAL[WAKEUP_SCHEDULE_LEN] =
{
    {Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 5},
    {Sleep::WakeupReason::REASON_CALL_HOME, 25}
};

/******************************************************************************
 * Thingsboard
 *****************************************************************************/
/** Thingsboard server URL */
const char TB_URL[] = ""; // Resolves to iot-dev.exm.gr

/******************************************************************************
 * Backend
 *****************************************************************************/
/** Url of backend server */
const char BACKEND_URL[] = "";

/** URL for plain HTTP rtc time sync 
 * GET requests to this URL must return just a timestamp and nothing else in its response body */
const char HTTP_TIME_SYNC_URL[] = "";

#endif