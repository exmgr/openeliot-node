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

/** Char buffer size for URLs */
const int URL_BUFFER_SIZE = 100;

/** Min/max allowed values for calling home interval (mins) */
const int CALL_HOME_INT_MINS_MIN = 1;           // 1 min
const int CALL_HOME_INT_MINS_MAX = 24 * 60 * 3; // 3 days

/** Min/max allowed values for water sensors measuring interval (mins) */
const int MEASURE_WATER_SENSORS_INT_MINS_MIN = 1;           // 1 min
const int MEASURE_WATER_SENSORS_INT_MINS_MAX = 24 * 60 * 1; // 1 day

/** If a timestamp is older than this, it is considered invalid. Must be set to any recent
 * value */
const unsigned long FAIL_CHECK_TIMESTAMP = 1563189899;

/******************************************************************************
 * Data storess
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
 * Event log
 *****************************************************************************/
/** Path in data store where log data is stored */
const char* const LOG_DATA_PATH = "/log";
/** Log data entries to group into a single data packet for submission */
const int LOG_ENTRIES_PER_SUBMIT_REQ = 50;
/** Arduino JSON doc size */
const int LOG_JSON_DOC_SIZE = 1024;
/** JSON output buffer size */
const int LOG_JSON_OUTPUT_BUFF_SIZE = 2048;

/******************************************************************************
 * Remote Control
 *****************************************************************************/
// JSon doc size for received remote config data
const int REMOTE_CONTROL_JSON_DOC_SIZE = 1024;
// Buffer for received data when sending remote control req
const int REMOTE_CONTROL_DATA_BUFF_SIZE = 512;

// Sent/received JSON parameter names
const char JSON_KEY_REMOTE_CONTROL_DATA_ID[] = "id";
const char JSON_KEY_MEASURE_WATER_SENSORS_INT[] = "ws_int";
const char JSON_KEY_CALL_HOME_INT[] = "ch_int";
const char JSON_KEY_REBOOT[] = "rebt";
const char JSON_KEY_OTA[] = "ota";

/******************************************************************************
 * Calling home
 *****************************************************************************/
/**
 * URL format for Thingsboard telemetry submission
 * Params: tb url, device access token
 */
const char TB_TELEMETRY_URL_FORMAT[] = "%s/api/v1/%s/telemetry";

/**
 * Format of URL where logs are submitted
 * Params: device id
 */
const char BACKEND_LOG_URL_FORMAT[] = "%s/?mode=log&id=%d";

/** Path where all requests to backend will be made */
const char BACKEND_REMOTE_CONTROL_PATH[] = "/remote/config/index.php";

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
const uint8_t GSM_FONA_SERIAL_PORT = 1;

const uint8_t GSM_FONA_TYPE_SIM7000E = 9;
const uint8_t GSM_FONA_TYPE_SIM7000G = 10;

/** Time to wait for GSM network connection before timeout */
const int GSM_DISCOVERY_TIMEOUT_MS = 30000;

/** Times to try to execute a command on the GSM module before failing */
const int GSM_TRIES = 3;
/** Time to delay between tries */
const int GSM_RETRY_DELAY_MS = 100;

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
/** TEMP: Compensation for adc. For some reason values returned are off  */
const int WATER_LEVEL_ADC_COMPENSATION_MV = 250; 
/** Max range in centimeters */
const int WATER_LEVEL_MAX_RANGE_CM = 765;
/** Times to measure (and calculate avg) */
const int WATER_LEVEL_MEASUREMENTS_COUNT = 5;
/** Delay in mS between measurements */
const int WATER_LEVEL_DELAY_BETWEEN_MEAS_MS = 50;
/** Millivolts per cm (read from analog output) */
const float WATER_LEVEL_MV_PER_CM = (float)3300 / WATER_LEVEL_MAX_RANGE_CM;
// CM per step
//const float WATER_LEVEL_CM_PER_STEP = ((3300/4096 + 200) / (3300 / 765));

/******************************************************************************
 * TB Diagnostic telemetry
 *****************************************************************************/
/** Arduino JSON doc size */
const int TB_DIAGNOSTICS_JSON_DOC_SIZE = 1024;
/** JSON output buffer size */
const int TB_DIAGNOSTICS_JSON_BUFF_SIZE = 2048;

/******************************************************************************
 * BME280
 *****************************************************************************/
const uint8_t BME280_I2C_ADDR = 0x77; // Default is 0x77

/******************************************************************************
 * Pins
 *****************************************************************************/
/** GSM module */
const uint8_t PIN_GSM_TX = 22;
const uint8_t PIN_GSM_RX = 21;
const uint8_t PIN_GSM_PWR = 5; // Power control

/** I2C pins for the main I2C bus */
const uint8_t PIN_I2C1_SDA = 32; // Nano pin A4
const uint8_t PIN_I2C1_SCL = 26; // Nano pin A5 

/** I2C pins for the I2C-SDI12 adapter */
const uint8_t PIN_SDI12_I2C1_SDA = 12;
const uint8_t PIN_SDI12_I2C1_SCL = 13;

/** Water quality sensor and SDI12 adapter power is controlled from the same pin */
const uint8_t PIN_WATER_QUALITY_PWR = 25;

/** Ultrasonic Water Level sensor */
const uint8_t PIN_WATER_LEVEL_PWR = 14;
const uint8_t PIN_WATER_LEVEL_ANALOG = 36;

#endif