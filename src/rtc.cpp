#include <ctime>
#include <Wire.h>
#include <RtcDS3231.h>
#include "Arduino.h"
#include "rtc.h"
#include "gsm.h"
#include "const.h"
#include "sys/time.h"
#include "log.h"
#include "utils.h"

namespace RTC
{
    //
    // Private vars
    //
    RtcDS3231<TwoWire> _ext_rtc(Wire);

    //
    // Private functions
    //

    /******************************************************************************
     * Initialization
     *****************************************************************************/
    RetResult init()
    {
        RetResult ret = RET_OK;
        if(USE_EXTERNAL_RTC)
        {
            ret = init_external_rtc();
        }

        return ret;
    }

    /******************************************************************************
     * Update ESP32 internal RTC (which is the main source of time keeping for the
     * whole application).
     * Flow:
     * 1. Sync GSM module RTC from NTP, then update system time from GSM module
     * 2. Update system time from plain HTTP
     * 3. Update system time from external RTC (if enabled and returned value valid)
     * 4. Update system time from GSM module RTC which has GSM time (because NTP failed)
     * 
     * @return RET_ERROR if all of the above methods failed, and there is no valid
     *                   system time
     *****************************************************************************/
    RetResult sync()
    {
        RetResult ret = RET_ERROR;
        
        // No need to update external RTC because this is where we got the time from
        bool update_ext_rtc = true;

        GSM::on();
        
        if(GSM::connect_persist() == RET_OK)
        {
            // Try to sync GSM module's RTC from NTP
            if(sync_gsm_rtc_from_ntp() == RET_OK)
            {
                // All good, update system time
                if(sync_time_from_gsm_rtc() == RET_OK)
                {
                    Utils::serial_style(STYLE_BLUE);
                    Serial.println(F("System time synced with NTP."));
                    Utils::serial_style(STYLE_RESET);
                    ret = RET_OK;
                }
                else
                {
                    Serial.println(F("Synctime time from NTP failed."));
                }
            }
            else
            {
                // Get time from plain HTTP
                if(sync_time_from_http() == RET_OK)
                {
                    Utils::serial_style(STYLE_BLUE);
                    Serial.println(F("System time synced with HTTP."));
                    Utils::serial_style(STYLE_RESET);
                    ret = RET_OK;
                }
                else
                {
                    Serial.println(F("Synctime time from HTTP failed."));

                    // Get time from external RTC (if enabled)
                    if(USE_EXTERNAL_RTC && sync_time_from_ext_rtc() == RET_OK)
                    {
                        Utils::serial_style(STYLE_BLUE);
                        Serial.println(F("System time synced with external RTC."));
                        Utils::serial_style(STYLE_RESET);

                        update_ext_rtc = false;
                        ret = RET_OK;
                    }
                    else
                    {
                        Serial.println(F("Synctime time from external RTC failed."));

                        // At this point, syncing GSM RTC from NTP has faield, so time returned from
                        // the gsm module is GSM time, (as long as there is network connectivity)
                        if(sync_time_from_gsm_rtc() == RET_OK)
                        {
                            Utils::serial_style(STYLE_BLUE);
                            Serial.println(F("System time synced with GSM time."));
                            Utils::serial_style(STYLE_RESET);
                            ret = RET_OK;
                        }
                        else
                        {
                            Serial.println(F("Synctime time GSM time failed."));
                        }
                    }
                }
            }
        }
        else
        {
            // Get time from external RTC (if enabled)
            if(USE_EXTERNAL_RTC && sync_time_from_ext_rtc() == RET_OK)
            {
                update_ext_rtc = false;
                ret = RET_OK;
            }
            else
            {
                Serial.println(F("Synctime time from external RTC failed."));
            }
        }

        GSM::off();

        // If time didn't come from external RTC, then update RTC from system time
        // (only if getting time succeeded)
        if(USE_EXTERNAL_RTC && update_ext_rtc && ret == RET_OK)
        {
            set_external_rtc_time(time(NULL));
        }

        return ret;
    }

    /******************************************************************************
    * Update GSM module's RTC from NTP
    ******************************************************************************/
    RetResult sync_gsm_rtc_from_ntp()
    {
        RetResult ret = RET_ERROR;
        int tries = GSM_TRIES;
        Serial.println(F("Syncing GSM module time with NTP."));

        //
        // Try to update GSM module time from NTP
        // Note: If GPRS fails to connect, module returns GSM time which may have 
        // wrong timezone
        while(tries--)
        {
            ret = GSM::update_ntp_time();
            if(ret != RET_OK && tries - 1)
            {
                Serial.println(F("Retrying..."));

                Serial.print(F("Tries: "));
                Serial.println(tries);
                delay(GSM_RETRY_DELAY_MS);
            }
            else
            {
                break;
            }
        }

        return ret;
    }

    /******************************************************************************
     * Get time from GSM module and update ESP32 internal RTC
     * Time in GSM could be synced from NTP (if NTP sync was run and succeeded) or
     * could be GSM time.
     *****************************************************************************/
    RetResult sync_time_from_gsm_rtc()
    {
        int tries = GSM_TRIES;

        //
        // Get time from GSM and set system time
        //
        tm tm_now = {0};
        tries = GSM_TRIES;

        Serial.println(F("Getting time from GSM module."));

        RetResult ret = RET_ERROR;
        while(tries--)
        {
            ret = GSM::get_time(&tm_now);
            if(ret != RET_OK && tries - 1)
            {
                Serial.println(F("Retrying..."));
                delay(GSM_RETRY_DELAY_MS);
            }
            else
            {
                break;
            }
        }
        if(ret != RET_OK)
            return ret;

        if(set_system_time(mktime(&tm_now)) == RET_ERROR)
        {
            return RET_ERROR;
        }

        return RET_OK;

        // //
        // // Update external RTC
        // //
        // if(USE_EXTERNAL_RTC)
        // {
        //     if(init_external_rtc() == RET_OK)
        //     {
        //         Serial.print(F("Updating external RTC from GSM: "));
        //         Serial.println(time(NULL));

        //         if(set_external_rtc_time(time(NULL)) != RET_OK)
        //         {
        //             Serial.println(F("Could not update external RTC time."));    
        //         }
        //     }
        //     else
        //     {
        //         Serial.println(F("Could not init external RTC"));
        //     }
        // }
      
        return RET_OK;
    }

    /******************************************************************************
    * Get time from backend server with a single GET request that returns current
    * timestamp in its body. Keep track of time passed since the req to compensate.
    * Body must contain only the timestamp.
    * Keeps track of time it took for the req to execute, to offset final timestamp
    * and obtain semi-accurate time
    ******************************************************************************/
    RetResult sync_time_from_http()
    {
        Serial.println(F("Syncing time from HTTP"));

        uint32_t start_time_ms = millis(), end_time_ms = 0;

        char resp[20] = "";
        uint16_t status_code = 0, resp_len = 0;
        if(GSM::get_req((char*)HTTP_TIME_SYNC_URL, &status_code, &resp_len, resp, sizeof(resp)) == RET_ERROR)
        {
            Serial.println(F("HTTP request failed"));
            return RET_ERROR;
        }
        end_time_ms = millis();

        resp[sizeof(resp) - 1] = '\0';

        Serial.print(F("Received: "));
        Serial.println(resp);

        // If not 10 chars, not a timestamp
        if(strlen(resp) != 10)
        {
            Serial.print(F("Data received is not timestamp: "));
            Serial.println(resp);
            return RET_ERROR;
        }

        unsigned long timestamp = 0;
        sscanf(resp, "%u", &timestamp);

        int offset_sec = round((float)(end_time_ms - start_time_ms) / 1000);
        Serial.print(F("Offsetting timestamp to compensate for req time (s): "));
        Serial.println(offset_sec, DEC);

        timestamp -= offset_sec;

        if(timestamp < FAIL_CHECK_TIMESTAMP)
        {
            Serial.print(F("Invalid timestamp received or resp. not a timestamp: "));
            Serial.println(timestamp, DEC);
            return RET_ERROR;
        }

        // Update system time
        if(set_system_time(timestamp) == RET_ERROR)
        {
            Serial.print(F("Could not update system time with timestamp: "));
            Serial.println(timestamp, DEC);
            return RET_ERROR;
        }

        return RET_OK;
    }

    /******************************************************************************
    * Update system time from external RTC
    ******************************************************************************/
    RetResult sync_time_from_ext_rtc()
    {
        uint32_t ext_rtc_tstamp = _ext_rtc.GetDateTime().Epoch32Time();

        if(ext_rtc_tstamp < FAIL_CHECK_TIMESTAMP)
        {
            Serial.print(F("Got invalid timestamp from ext rtc: "));
            Serial.println(ext_rtc_tstamp, DEC);
            return RET_ERROR;
        }

        Serial.print(F("Setting system time from ext RTC: "));
        Serial.println(ext_rtc_tstamp);

        timeval tv_now = {0};
        tv_now.tv_sec = ext_rtc_tstamp;

        // Set system time
        if(settimeofday(&tv_now, NULL) != 0)
        {
            Serial.println(F("Could not set system time."));
            return RET_ERROR;
        }

        return RET_OK;
    }

    /******************************************************************************
    * Init external RTC
    ******************************************************************************/
    RetResult init_external_rtc()
    {
        // Retry several times before failing
        // int tries = 3;
        // if(Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL) == false)
        // {
        //     Serial.println(F("Could not begin I2C comms with ext RTC."));
        //     if(tries--)
        //     {
        //         Serial.println(F("Retrying..."));
        //     }
        //     else
        //     {
        //         Serial.println(F("Failed. Aborting."));
        //         return RET_ERROR;
        //     }
        // }

        if(!_ext_rtc.IsDateTimeValid())
        {
            Utils::serial_style(STYLE_RED);
            Serial.println(F("RTC osc stop detected."));
            Utils::serial_style(STYLE_RESET);

            if (_ext_rtc.LastError() != 0)
            {
                Serial.printf("Could not communicate with ext RTC.");
                return RET_ERROR;
            }
            else
            {
                Serial.println(F("RTC invalid time but no error code."));
                return RET_ERROR;
            }
        }

        if (!_ext_rtc.GetIsRunning())
        {
            Serial.println("Ext RTC was not running, starting.");
            _ext_rtc.SetIsRunning(true);
        }

        _ext_rtc.Enable32kHzPin(false);
        _ext_rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 

        Serial.print(F("External RTC init complete. Time: "));
        Serial.println(_ext_rtc.GetDateTime().Epoch32Time());
 
        return RET_OK;
    }

    /******************************************************************************
    * Set external RTC time
    ******************************************************************************/
    RetResult set_external_rtc_time(uint32_t timestamp)
    {
        // External RTC counts time in secs from 2000, subtract offset
        uint32_t secs_since_2000 = timestamp - SECONDS_IN_2000;
        
        _ext_rtc.SetDateTime(secs_since_2000);
        if(_ext_rtc.LastError() != 0)
        {
            Serial.print(F("Could not set ext RTC time to: "));
            Serial.println(timestamp);
            Serial.print("(");
            Serial.print(secs_since_2000, DEC);
            Serial.println(")");

            return RET_ERROR;
        }
        else
        {
            Serial.print(F("External RTC time set: "));
            Serial.println(timestamp, DEC);

            return RET_OK;
        }
        
    }

    /******************************************************************************
    * Set system time
    ******************************************************************************/
    RetResult set_system_time(uint32_t timestamp)
    {
        timeval tval_now = {0};

        // Check if time valid
        tval_now.tv_sec = timestamp;
        if(tval_now.tv_sec == -1)
        {
            Serial.println(F("Cannot set time to invalid timestamp."));
            return RET_ERROR;
        }

        // Set system time
        if(settimeofday(&tval_now, NULL) != 0)
        {
            Serial.println(F("Could not set system time"));
            return RET_ERROR;
        }

        return RET_OK;
    }

    /******************************************************************************
     * Get time from external RTC
     *****************************************************************************/
    uint32_t get_external_rtc_timestamp()
    {
        return _ext_rtc.GetDateTime().Epoch32Time();
    }

    /******************************************************************************
    * Get current time stamp
    ******************************************************************************/
    uint32_t get_timestamp()
    {
        return time(NULL);
    }

    /******************************************************************************
     * Print current time to serial output
     *****************************************************************************/
    void print_time()
    {
        Serial.print("Time: ");
        time_t cur_tstamp = time(NULL);
        
        Serial.print(ctime(&cur_tstamp));
        Serial.print("(");
        Serial.print(cur_tstamp, DEC);
        Serial.println(")");
    }
}