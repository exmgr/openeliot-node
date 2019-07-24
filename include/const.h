#ifndef CONST_H
#define CONST_H
#include <inttypes.h>
#include "sleep.h"

/******************************************************************************
 * Misc
 *****************************************************************************/
/** Serial output baud */
const int SERIAL_MONITOR_BAUD = 115200;

// Seconds from unix epoch to 01/01/2000 00:00:00. Used to convert time from
// RTC to UNIX timestamp used by ESP32s internal RTC
const int SECONDS_IN_2000 = 946684800;

/** Buffer size for URLs */
const int URL_BUFFER_SIZE = 128;

/** Buffer size for URL's host part (used when exploding URLs) */
const int URL_HOST_BUFFER_SIZE = 40;

/** Larger buffer size for storing larger URL with many params (eg. thingsboard shared attrs) */
const int URL_BUFFER_SIZE_LARGE = 256;

/** Min/max allowed values for calling home interval (mins) */
const int CALL_HOME_INT_MINS_MIN = 1;           // 1 min
const int CALL_HOME_INT_MINS_MAX = 24 * 60 * 3; // 3 days

/** Min/max allowed values for water sensors measuring interval (mins) */
const int MEASURE_WATER_SENSORS_INT_MINS_MIN = 1;           // 1 min
const int MEASURE_WATER_SENSORS_INT_MINS_MAX = 24 * 60 * 1; // 1 day

/** If a timestamp is older than this, it is considered invalid. Must be set to any recent
 * value */
const unsigned long FAIL_CHECK_TIMESTAMP = 1567157191;

/** Samples to average when reading battery voltage with internal ADC */
const int ADC_BATTERY_LEVEL_SAMPLES = 50;

/******************************************************************************
 * Data stores
 *****************************************************************************/
/** Maximum length for file path buffers */
const int FILE_PATH_BUFFER_SIZE = 25;

/** Max tries when looking for an unused filename before failing */
const int FILENAME_POSTFIX_MAX = 100;

/******************************************************************************
 * Sensor data
 *****************************************************************************/
/** Path in data store where sensor data is stored */
const char* const SENSOR_DATA_PATH = "/sd";

/** Arduino JSON doc size */
const int SENSOR_DATA_JSON_DOC_SIZE = 1024;
/** JSON output buffer size */
const int SENSOR_DATA_JSON_OUTPUT_BUFF_SIZE = 2048;
/** Sensor data entries to group into a single json packet for submission */
const int SENSOR_DATA_ENTRIES_PER_SUBMIT_REQ = 8;


/******************************************************************************
 * Log
 *****************************************************************************/
/** Path in data store where log data is stored */
const char* const LOG_DATA_PATH = "/log";
/** Log data entries to group into a single data packet for submission */
const int LOG_ENTRIES_PER_SUBMIT_REQ = 8;
/** Arduino JSON doc size */
const int LOG_JSON_DOC_SIZE = 1024;
/** JSON output buffer size */
const int LOG_JSON_OUTPUT_BUFF_SIZE = 1024;

/******************************************************************************
 * Remote Control
 *****************************************************************************/
/** JSON doc size for received remote config data */
const int REMOTE_CONTROL_JSON_DOC_SIZE = 1024;


// Sent/received JSON parameter names
const char TB_KEY_REMOTE_CONTROL_DATA_ID[] = "data_id";
const char TB_KEY_MEASURE_WATER_SENSORS_INT[] = "ws_int";
const char TB_KEY_CALL_HOME_INT[] = "ch_int";
const char TB_KEY_DO_REBOOT[] = "do_reboot";
const char TB_KEY_DO_OTA[] = "do_ota";
const char TB_KEY_FORMAT_SPIFFS[] = "do_format";
const char TB_KEY_FW_URL[] = "fw_url";
const char TB_KEY_FW_VERSION[] = "fw_v";
const char TB_KEY_FW_MD5[] = "fw_md5";

/******************************************************************************
 * Client attributes
 *****************************************************************************/
/** JSON doc size for client attributes request body */
const int CLIENT_ATTRIBUTES_JSON_DOC_SIZE = 1024;

/******************************************************************************
 * Calling home
 *****************************************************************************/
/**
 * TB telemetry API URL
 * Params: tb url, device access token
 */
const char TB_TELEMETRY_URL_FORMAT[] = "/api/v1/%s/telemetry";

/**
 * TB API URL for publishing client attributes
 * Params: device access token
 */
const char TB_CLIENT_ATTRIBUTES_URL_FORMAT[] = "/api/v1/%s/attributes?clientKeys=cur_fw_v,cur_ws_int,cur_ch_int";

/**
 * TB API URL for getting shared attributes for remote control
 * Params: device access token
*/
const char TB_SHARED_ATTRIBUTES_URL_FORMAT[] = "/api/v1/%s/attributes?sharedKeys=data_id,ch_int,fw_v,fw_url,fw_md5,ws_int,ch_int,do_ota,do_rebt,do_format";


/** URL for plain HTTP rtc time sync 
 * GET requests to this URL must return just a timestamp and nothing else in its response body */
const char HTTP_TIME_SYNC_URL[] = "";

/******************************************************************************
* DeviceConfig store
******************************************************************************/
/** Preferences api namespace name for DeviceConfig. Also used as the single key
 * where config struct is stored */
const char DEVICE_CONFIG_NVS_NAMESPACE_NAME[] = "DevConf";

/******************************************************************************
 * GSM
 *****************************************************************************/
/** HardwareSerial port to use for communication with the GSM module */
const uint8_t GSM_SERIAL_PORT = 1;

const int GSM_SERIAL_BAUD = 115200;

/** Time to wait for GSM network connection before timeout */
const int GSM_DISCOVERY_TIMEOUT_MS = 30000;

/** Times to try to execute a command on the GSM module before failing */
const int GSM_TRIES = 3;
/** Time to delay between tries */
const int GSM_RETRY_DELAY_MS = 100;

/** TinyGSM buffer size - Used by tinygsm*/
#define TINY_GSM_RX_BUFFER	1024

/******************************************************************************
 * HTTP
 *****************************************************************************/
/** Generic global HTTP response buffer size. Buffer is large and thus stored in global scope 
 * to avoid stack overflow.
*/
const int GLOBAL_HTTP_RESPONSE_BUFFER_LEN = 4096;

/** Timeout when reading http client stream */
const int HTTP_CLIENT_STREAM_READ_TIMEOUT = 3000;

/******************************************************************************
 * SDI12 adapter (i2c slave)
 *****************************************************************************/
/** Receive buffer size. Must be large enough to fit a single response */ 
const int SDI12_ADAPTER_BUFF_SIZE = 48;

/** I2C address of adapter */
const int SDI12_I2C_ADDRESS = 10;

/** Time to wait when reading response, before aborting. */
const int SDI12_ADAPTER_READ_TIMEOUT_MS = 1000;

/** Time to wait after sending a command and before reading back the response */
const int SDI12_COMMAND_WAIT_TIME_MS = 500;

/** Number of I2C bus to use. Must be only device on bus */
const uint8_t SDI12_ADAPTER_I2C_BUS = 1;

/******************************************************************************
 * Water Quality Sensors
 *****************************************************************************/
/** Time to wait for sensor to boot after powering it up */
const int WATER_QUALITY_BOOT_DELAY = 2000;

/** Max time to wait for water quality sensor to prepare measurements after a 
 * measure command. Used in case sensor returns garable values, to prevent
 * waiting for long amounts of time. Value must be adapted to water quality
 * sensor configuration (number of measurements). Allow a few seconds of error */
const int WATER_QUALITY_MEASURE_WAIT_SEC_MAX = 15;

/** Number of measurement values expected from the sensor */
const int WATER_QUALITY_NUMBER_OF_MEASUREMENTS = 5;

/** Number of extra seconds to wait for measurement on top of what the sensors says */
const int WATER_QUALITY_MEASURE_EXTRA_WAIT_SECS = 1;

/** Number of seconds to wait before retrying after an error */
const int WATER_QUALITY_RETRY_WAIT_MS = 1000;

/******************************************************************************
 * Water Level sensor
 *****************************************************************************/

// General
/** Times to measure (and calculate avg) */
const int WATER_LEVEL_MEASUREMENTS_COUNT = 10;
/** Delay in mS between measurements */
const int WATER_LEVEL_DELAY_BETWEEN_MEAS_MS = 5;
/** Max range in centimeters */
const int WATER_LEVEL_MAX_RANGE_MM = 9999;

// PWM channel only
/** Timeout when waiting for PWM pulse */
const int WATER_LEVEL_PWM_TIMEOUT_MS = 500;

/** Sensor returns max range when no target detected within range
Since PWM has an offset a tolerance is taken into account when ignoring invalid values */
const int WATER_LEVEL_PWM_MAX_VAL_TOL = 30;

/** 
 * Maximum failures to acquire valid values when reading sensor before aborting
 * measurement completely
 **/
const int WATER_LEVEL_PWM_FAILED_MEAS_LIMIT = 3;

// Analog channel only
/** Millivolts per cm */
const float WATER_LEVEL_MV_PER_MM = (float)3300 / WATER_LEVEL_MAX_RANGE_MM;

/******************************************************************************
 * BME280 (internal environment sensor 1)
 *****************************************************************************/
const uint8_t BME280_I2C_ADDR1 = 0x76; 
const uint8_t BME280_I2C_ADDR2 = 0x77; 

/******************************************************************************
 * IP5306 (TCall PMU ic)
 *****************************************************************************/
const uint8_t IP5306_I2C_ADDR = 0x75;
const uint8_t IP5306_REG_SYS_CTL0 = 0x00;
const uint8_t IP5306_BOOST_FLAGS = 0x37;  // 0x37 = 0b110111 TCALL example
      
  /*
 [1]      Boost EN (default 1)            [EXM note: if 0 ESP32 will not boot from battery]
 [1]      Charger EN (1)                  [EXM note: did  not observe difference]
 [1]      Reserved (1)                    [EXM note: did  not observe difference]
 [1]      Insert load auto power EN (1)   [EXM note: did  not observe difference]
 [1]      BOOST output normally open ( 1) [EXM note: if 0 will shutdown on ESP32 sleep after 32s]
 [1]      Key off EN: (0)                 [EXM note: could not detect difference]
  */

 /******************************************************************************
 * TB Diagnostic telemetry
 *****************************************************************************/
/** Arduino JSON doc size */
const int TB_DIAGNOSTICS_JSON_DOC_SIZE = 1024;
/** JSON output buffer size */
const int TB_DIAGNOSTICS_JSON_BUFF_SIZE = 2048;

/******************************************************************************
 * Things board timeseries/attribute keys
 *****************************************************************************/
//
// Shared attributes
//

//
// Client attributes
//
const char TB_ATTR_CUR_FW_V[] = "cur_fw_v";
const char TB_ATTR_CUR_WS_INT[] = "cur_ws_int";
const char TB_ATTR_CUR_CH_INT[] = "cur_ch_int";

//
// Diagnostic telemetry
//
// TODO: Not used yet
const char TB_KEY_GSM_NETWORK_DISCOVERY_FAILED[] = "d_gsm_conn_fail";
const char TB_KEY_GSM_GPRS_CONNECTION_FAILED[] = "d_gprs_conn_fail";
const char TB_KEY_GSGSM_RSSIM_RSSI[] = "d_gsm_rssi";
const char TB_KEY_NTP_TIME_SYNC_FAILED[] = "d_ntp_fail";
const char TB_KEY_SENSOR_DATA_SUBMITTED_TOTAL[] = "d_sd_total_rec";
const char TB_KEY_SENSOR_DATA_SUBMITTED_CRC_FAILURES[] = "d_sd_crc";
const char TB_KEY_SENSOR_DATA_SUBMISSION_ERRORS_TOTAL_REQ[] = "d_sd_total_req";
const char TB_KEY_SENSOR_DATA_SUBMISSION_ERRORS_FAILED_REQ[] = "d_sd_failed_req";
const char TB_KEY_BATTERY_MV[] = "d_bat_mv";
const char TB_KEY_BATTERY_PCT[] = "d_bat_p";
const char TB_KEY_BATTERY_GSM_MV[] = "d_bat_gsm_mv";
const char TB_KEY_BATTERY_GSM_PCT[] = "d_bat_gsm_p";
const char TB_KEY_INT_ENV_SENSOR1_TEMP[] = "d_temp";
const char TB_KEY_INT_ENV_SENSOR1_HUM[] = "d_hum";
const char TB_KEY_SLEEP[] = "d_tm_awake_s";
const char TB_KEY_WAKEUP[] = "d_wakeup";
const char TB_KEY_BOOT[] = "d_boot";

/******************************************************************************
 * Battery
 *****************************************************************************/
// TODO: Move to struct.h pr battery.h
enum BATTERY_MODE
{
	BATTERY_MODE_NORMAL = 1,
	BATTERY_MODE_LOW,
	BATTERY_MODE_SLEEP_CHARGE
};

struct BATTERY_PCT_LUT_ENTRY
{
	uint8_t pct;
	uint16_t mv;
};

/**
 * Look up table for lithium battery SoC - voltage
 * Used when measuring battery state with ADC
 */
const BATTERY_PCT_LUT_ENTRY BATTERY_PCT_LUT[] = {
	{0, 2750}, {1, 3050}, {2, 3230}, {3, 3340}, {4, 3430}, {5, 3490}, {6, 3530}, {7, 3550}, {8, 3560}, {9, 3570}, 
	{10, 3580}, {13, 3590}, {14, 3600}, {16, 3610}, {18, 3630}, {23, 3640}, {25, 3650}, {27, 3660}, {31, 3670}, {36, 3680}, 
	{41, 3690}, {43, 3700}, {47, 3710}, {51, 3720}, {53, 3730}, {55, 3740}, {59, 3750}, {62, 3760}, {63, 3770}, {65, 3780}, 
	{66, 3790}, {67, 3800}, {68, 3810}, {70, 3820}, {73, 3830}, {74, 3840}, {76, 3850}, {78, 3860}, {79, 3870}, {81, 3880}, 
	{84, 3890}, {85, 3900}, {86, 3920}, {88, 3930}, {90, 3940}, {91, 3950}, {92, 3960}, {94, 3970}, {95, 3980}, {96, 3990}, 
	{98, 4000}, {99, 4010}, {100, 4030}
};



#endif