#include <ctime>
#include <string>
#include "app_config.h"
#include "const.h"
#include "gsm.h"
#include "rtc.h"
#include "struct.h"
#include "utils.h"
#include "log.h"

namespace GSM
{

//
// Private vars
//
/** Hardware serial port to be used by the FONA lib */
HardwareSerial _fona_serial(GSM_FONA_SERIAL_PORT);
/** Fona lib object */
Adafruit_FONA_LTE _fona = Adafruit_FONA_LTE();
/** When serial communcation is established with fona, set to true. When OFF, set back to false.
 * Checking if(_fona_serial) doesn't seem to do anything */
bool _fona_serial_open = false;

//
// Private functions
//
void power_cycle();
bool is_fona_serial_open();

/******************************************************************************
 * Init GSM functions 
 ******************************************************************************/
void init()
{
    // Make sure its off
    off();
}

/*****************************************************************************
* Power ON
*****************************************************************************/
RetResult on()
{
    int ret = 0, tries = 0;

    // Turn ON through DFRobot solar manager
    digitalWrite(PIN_GSM_PWR, HIGH);
    delay(500);    

    // If fona already open, calling begin on it again can cause problems
    // Can fona serial become unavailable for some reason? Check if it is still there
    if(!_fona_serial)
    {
        Serial.println(F("Fona serial begin."));
        _fona_serial.begin(115200, SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
    }
    else
    {
        Serial.println(F("Fona serial already open."));
    }

    // Change baud rate to 9600
    Serial.println("Change baud rate to 9600.");
    _fona_serial.println("AT+IPR=9600");
    delay(100);
    _fona_serial.begin(9600, SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
    delay(100);

    // Serial comms have began
    _fona_serial_open = true;

    //
    // Start and configure fona
    // Try X times before aborting
    tries = GSM_TRIES;
    do
    {
        Serial.println(F("Fona begin."));
        if (!(ret = _fona.begin(_fona_serial)))
        {
            Serial.println(F("Couldn't connect to GSM module."));
            return RET_ERROR;
        }
        else
            break;

    }while(--tries);
    
    if(!ret)
        return RET_ERROR;

    return RET_OK;
}

/*****************************************************************************
* Power OFF
*****************************************************************************/
RetResult off()
{
    // _fona.powerDown();
    // delay(1000);
    
    digitalWrite(PIN_GSM_PWR, LOW);

    _fona_serial_open = false;

    return RET_OK;
}

/******************************************************************************
 * Connect to the network and enable GPRS
 *****************************************************************************/
RetResult connect()
{
    Serial.println(F("Starting GSM."));
    int tries = 0, ret = 0;

    // Get fona type. Must be SIM7000E or SIM7000G
    uint8_t type = _fona.type();
    if (type == GSM_FONA_TYPE_SIM7000E || type == GSM_FONA_TYPE_SIM7000G)
    {
        Serial.println(F("Found SIM7000E/G"));
    }
    else
    {
        Serial.print(F("GSM module is not SIM7000E/G. Type: "));
        Serial.println(type, DEC);
        return RET_ERROR;
    }

    // Print IMEI because we can
    char imei[16] = {0};
    if (_fona.getIMEI(imei))
    {
        Serial.print(F("IMEI: "));
        Serial.println(imei);
    }

    // Set modem to full functionality
    _fona.setFunctionality(1); // AT+CFUN=1

    // GSM/MBIoT mode
    if(NBIOT_MODE)
    {
        Utils::serial_style(STYLE_BLUE);
        Serial.println("NBIoT mode");
        Utils::serial_style(STYLE_RESET);

        // Mode = NBIoT
        _fona.setPreferredMode(38);
        // LTE Mode = NBIoT
        _fona.setPreferredLTEMode(2);
        _fona.setOperatingBand("NB-IOT", 20);
    }
    else
    {
        Serial.println("GSM mode");
        // GSM only
        _fona.setPreferredMode(13);
    }

    // Set APN from device descriptor
    const DeviceDescriptor *device_descriptor = Utils::get_device_descriptor();

    Serial.print(F("Setting APN: "));
    Serial.println(F(device_descriptor->apn));
    _fona.setNetworkSettings(F(device_descriptor->apn));

    _fona.setNetLED(true, 2, 500, 500);
    // In debug mode led blinks slowly, else its disabled completely
    // #ifdef DEBUG
    //     // On/off, mode, timer_on, timer_off (on release, turn off completely)
    //     _fona.setNetLED(true, 2, 500, 500);
    // #else
    //     // Disable network status LED
    //     _fona.setNetLED(false);
    // #endif
    // END

    //
    // Connect to network
    //
    tries = GSM_TRIES;
    bool success = false;
    uint32_t discovery_start_time = millis();

    // Check for network status change every X ms
    // If not connected within X seconds, connection failed
    Serial.println(F("Connecting to network."));
    while(millis() - discovery_start_time < GSM_DISCOVERY_TIMEOUT_MS)
    {
        if(_fona.getNetworkStatus() != 1 && _fona.getNetworkStatus() != 5)
        {
            Serial.print(".");
            delay(500);
        }
        else
        {
            success = true;
            break;
        }
    }

    if(!success)
    {
        Serial.println(F("Connection failed."));

		Log::log(Log::GSM_CONNECT_FAILED);
        return RET_ERROR;
    }
    else
    {
        Serial.println(F("Network connected!"));
    }

    int8_t rssi = get_rssi();;
    Utils::serial_style(STYLE_BLUE);
    Serial.print(F("RSSI: "));
    Serial.println(rssi, DEC);
    Utils::serial_style(STYLE_RESET);

    // Adafruit says to wait a few secs for stabilization (?)
    delay(3000);

    // END

    //
    // Enable GPRS
    //
    if(_fona.GPRSstate() == 1)
    {
        Serial.println("GPRS already enabled");
    }
    else
    {
        Serial.println(F("Enabling GPRS."));
        tries = GSM_TRIES;
        while(tries--)
        {

            if(!(ret = _fona.enableGPRS(true)))
            {
                Serial.println(F("Could not enable GPRS"));

                if(tries)
                {
                    Serial.println(F("Disabling GPRS and retrying."));
                }
            }
            else
                break;

            // Disable GPRS before reenabling
            _fona.enableGPRS(false);
        }

        if(!ret)
        {
            Serial.println(F("Could not enable GPRS. Aborting."));
            return RET_ERROR;
        }
        else
        {
            Serial.println(F("GPRS enabled."));
        }
    }

    Serial.println(F("Connected."));

    // END

    return RET_OK;
}

/******************************************************************************
 * Calls connect() X times until it succeeds and power cycles GSM module
 * inbetween failures
 ******************************************************************************/
RetResult connect_persist()
{
    int tries = GSM_TRIES;
    int success = false;

    // Try to connect X times before aborting, power cycling in between
    while(tries--)
    {
        if(connect() == RET_ERROR)
        {
            Serial.print(F("Connection failed."));

            if(tries)
            {
                Serial.println(F("Cycling power and retrying."));
            }
            power_cycle();
        }
        else
        {
            success = true;
            break;
        }   
    }

    if(!success)
    {
        Serial.println(F("Could not connect. Aborting."));

        return RET_ERROR;
    }
    else
        return RET_OK;
}

/*****************************************************************************
* Power cycle
*****************************************************************************/
void power_cycle()
{
    digitalWrite(PIN_GSM_PWR, LOW);
    delay(500);
    digitalWrite(PIN_GSM_PWR, HIGH);
}

/*****************************************************************************
* Update time from NTP
*****************************************************************************/
RetResult update_ntp_time()
{
    Serial.print(F("Updating time from: "));
    Serial.println(NTP_SERVER);

    // Toggle GPRS to make sure it returns NTP time and not GSM time
    enable_gprs(false);
    if(enable_gprs(true) != RET_OK)
       return RET_ERROR;

    if(!_fona.enableNTPTimeSync(true, F(NTP_SERVER)))
        return RET_ERROR;

    return RET_OK;
}

/*****************************************************************************
* Get time from module
*****************************************************************************/
RetResult get_time(tm *out)
{
    if(!is_fona_serial_open())
    {
        return RET_ERROR;
    }

    char buff[26] = "";

    // Returned format: "30/08/87,05:49:54+00"
    if(!_fona.getTime(buff, sizeof(buff)))
    {
        Serial.println(F("Could not get time from FONA."));
        return RET_ERROR;
    }

    if(0 == sscanf(buff, "\"%02d/%02d/%02d,%02d:%02d:%02d",
        &(out->tm_year), &(out->tm_mon), &(out->tm_mday),
        &(out->tm_hour), &(out->tm_min), &(out->tm_sec)))
    {
        Serial.print(F("Could not parse FONA time: "));
        Serial.println(buff);

        // 0 vars parsed
        return RET_ERROR;
    }

    // Year in GSM module counts from 2000, must count from 1900
    out->tm_year += 100;
    // Month in GSM module counts from 01, must count from 00
    out->tm_mon--;

    time_t gsm_epoch = mktime(out);
    Serial.print(F("Parsed time in GSM module: "));
    Serial.println(ctime(&gsm_epoch));

    return RET_OK;
}

/******************************************************************************
 * Enable/disable GPRS
 *****************************************************************************/
RetResult enable_gprs(bool enable)
{
    if(_fona.enableGPRS(enable))
        return RET_OK;
    else
        return RET_ERROR;
}

/******************************************************************************
* Convert value returned by SIM into dBm and return
* @return RSSI in dBm
******************************************************************************/
int get_rssi()
{
    if(!is_fona_serial_open())
        return 0;

    uint8_t rssi_sim = _fona.getRSSI();
    int8_t rssi = 0;
    
    if(rssi_sim == 0)
        rssi = -115;
    if(rssi_sim == 1)
        rssi = -111;
    if(rssi_sim == 31)
        rssi = -52;
    if((rssi_sim >= 2) && (rssi_sim <= 30))
        rssi = map(rssi_sim, 2, 30, -110, -54);

    return rssi;
}

/******************************************************************************
 * Get battery info from module
 * @param 
 *****************************************************************************/
RetResult get_battery_info(uint16_t* voltage, uint16_t* pct)
{
    if(!is_fona_serial_open())
    {
        return RET_ERROR;
    }

    if(!_fona.getBattVoltage(voltage))
    {
        Serial.println(F("Could not read voltage from GSM module"));
        return RET_ERROR;
    }

    if(!_fona.getBattPercent(pct))
    {
        Serial.println(F("Could not read battery pct from GSM module."));
        return RET_ERROR;
    }

    return RET_OK;
}

/******************************************************************************
 * Check if SIM card present by querying
 *****************************************************************************/
bool sim_card_present()
{
    char ccid[32] = "";

    // Bug in fona: module returns CME ERROR: 10 when sim card not present and fona
    // instead of returning len=0, returns 6 because it slices the error into "0R: 10"
    uint8_t len = _fona.getSIMCCID(ccid);

    return len < 10 ? false : true;
}

/******************************************************************************
 * Return true if fona serial port has been set up and communications established
 * Used to check if it is OK to talk to Fona. Talking to fona without checking
 * this might result in eceptions/
 *****************************************************************************/
bool is_fona_serial_open()
{
    return _fona_serial_open;
}

/*****************************************************************************
* Do a GET/POST request. If fails, toggle GPRS and retry
*****************************************************************************/
RetResult req(const char *url, const char *method, const char *req_data, int req_data_size, 
            char *resp_buffer, int resp_buffer_size)
{
    int tries = GSM_TRIES;
    int ret = 0;

    while(tries--)
    {
        if(!(ret = _fona.postData(method, url, req_data, "", req_data_size)))
        {
            Serial.println("Request failed. ");

            if(tries - 1)
            {
                Serial.println(F("Toggling GPRS and retrying..."));
                enable_gprs(false);
                delay(GSM_RETRY_DELAY_MS);
                enable_gprs(true);
                delay(GSM_RETRY_DELAY_MS);
            }
        }
        else
        {
            break;
        }
    }

    if(!ret)
    {
        Serial.println(F("Aborting"));
        return RET_ERROR;
    }
    
    Serial.println(F("Request success."));

    //
    // Get response
    //
    if(resp_buffer != NULL && resp_buffer_size > 1)
    {      
        Serial.println(F("Reading buffer"));

        delay(2000);
        
        while(_fona.available())
        {
            Serial.print("-");
            char c = _fona.read();
            Serial.print(c);
            Serial.println();
        }

        // Read 1 byte less, leave space for null termination
        // int read_bytes = fona.readBytes(resp_buffer, resp_buffer_size - 1);

        // Serial.print(F("Read bytes: "));
        // Serial.println(read_bytes, DEC);

        // Null terminate
        // resp_buffer[read_bytes] = '\0';

        // Serial.print(F("BUffer: "));
        // Serial.println(resp_bufferl);
    }

    return RET_OK;
}

/******************************************************************************
* Do a GET request
******************************************************************************/
RetResult get_req(char *url, uint16_t *status_code, uint16_t *resp_len, char *resp_buff, int resp_buff_size)
{
    if(!is_fona_serial_open)
    {
        return RET_ERROR;
    }

    int tries = GSM_TRIES;
    int ret = 0;

    while(tries--)
    {
        if(!(ret = _fona.HTTP_GET_start(url, status_code, resp_len)))
        {
            Serial.println("Request failed. ");

            if(tries - 1)
            {
                Serial.println(F("Toggling GPRS and retrying..."));
                enable_gprs(false);
                delay(GSM_RETRY_DELAY_MS);
                enable_gprs(true);
                delay(GSM_RETRY_DELAY_MS);
            }
        }
        else
        {
            break;
        }
    }

    if(!ret)
    {
        Serial.println(F("Aborting"));
        return RET_ERROR;
    }

    if(*resp_len < 1)
        return RET_ERROR;

    // Maximum amount of bytes to read is the smallest of: buffer size and actual response length
    // Treat output buffer size as 1 char smaller than provided, to make space for termination char
    int bytes_to_read = resp_buff_size - 1;
    if(bytes_to_read > *resp_len)
        bytes_to_read = *resp_len;

    int bytes_read = _fona.readBytes(resp_buff, bytes_to_read);

    _fona.HTTP_GET_end();

    resp_buff[bytes_read] = '\0';

    return RET_OK;
}


} // namespace GSM