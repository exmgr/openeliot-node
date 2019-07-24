#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include "struct.h"
#include "Arduino.h"
#include "sleep.h"

/** FW Version */
const int FW_VERSION = 100;

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
    {1, "00:00:00:00:00:00", "your_apn", "tb_token"}
};

/******************************************************************************
 * General
 *****************************************************************************/

// Main switches

/** Enable NBIoT mode. If false GSM is used */
const bool NBIOT_MODE = false;

/** When sleeping treat minutes as seconds. Used for debugging */
const bool SLEEP_MINS_AS_SECS = false;

/** Return dummy values when measuring water quality */
const bool MEASURE_DUMMY_WATER_QUALITY = false;

/** Return dummy values when measuring water level */
const bool MEASURE_DUMMY_WATER_LEVEL = false;

/** Use external RTC when syncing time */
const bool EXTERNAL_RTC_ENABLED = true;

/** Publish specific log codes as telemetry to thingsboard (diagnostic telemetry) */
const bool PUBLISH_TB_DIAGNOSTIC_TELEMETRY = true;

/** Print serial comms between the MCU and the GSM module (used by tinyGSM) */
#define PRINT_GSM_AT_COMMS false


/** Size of DataStore's buffer that holds uncommited data. Once this buffer
 is full, data is commited to flash */
const int DATA_STORE_BUFFER_ELEMENTS = 10;

/** NTP server used by GSM module for time sync */
const char NTP_SERVER[] PROGMEM = "pool.ntp.org";
// const char NTP_SERVER[] PROGMEM = "ntp.grnet.gr";
// const char NTP_SERVER[] PROGMEM = "gr.pool.ntp.org"; //returns local time (?)

/** 
 * Channel to use when capturing data from Water Level sensor 
 * 1: PWM
 * 2: ANALOG
 * 3: Serial
*/
const WaterLevelChannel WATER_LEVEL_INPUT_CHANNEL = WATER_LEVEL_CHANNEL_PWM;

/******************************************************************************
* Sleep/wake up schedules
******************************************************************************/
/** Default schedule used when battery levels normal and custom settings have
 * not been set by user yet
 */
const Sleep::WakeupScheduleEntry WAKEUP_SCHEDULE_DEFAULT[] =
{
    {Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 5},
    {Sleep::WakeupReason::REASON_CALL_HOME, 10}
};

/** Length of wake up schedule. Len must be known so it can be written to DeviceConfig */
const int WAKEUP_SCHEDULE_LEN = sizeof(WAKEUP_SCHEDULE_DEFAULT) / sizeof(WAKEUP_SCHEDULE_DEFAULT[0]);

/**
 * Used when battery levels critical (user/default settings ignored).
 * Current set schedule is overriden
 * */
const Sleep::WakeupScheduleEntry WAKEUP_SCHEDULE_BATT_LOW[WAKEUP_SCHEDULE_LEN] =
{
    {Sleep::WakeupReason::REASON_READ_WATER_SENSORS, 10},
    {Sleep::WakeupReason::REASON_CALL_HOME, 30}
};

/******************************************************************************
 * Power
 *****************************************************************************/
/**
 * From this level and below battery level is considered critical.
 * Battery level affects settings used (eg. measuring intervals)
*/
const int BATTERY_LEVEL_LOW = 50;

/**
 * Level at which device will sleep until its recharged.
 * When in this mode, device wakes up every SLEEP_CHARGE_CHECK_INT_MINS to check
 * if battery reached BATTERY_LEVEL_SLEEP_RECHARGED.
*/
const int BATTERY_LEVEL_SLEEP_CHARGE = 20;
const int BATTERY_LEVEL_SLEEP_RECHARGED = 30;

const int SLEEP_CHARGE_CHECK_INT_MINS = 60;

/******************************************************************************
 * Thingsboard
 *****************************************************************************/
/** Thingsboard server URL */
const char TB_SERVER[] = ""; // openeliot.exm.gr

const int TB_PORT = 8080;

#endif