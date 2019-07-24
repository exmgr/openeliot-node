#include "battery.h"
#include "MAX17043.h"
#include "const.h"
#include "log.h"
#include "utils.h"
#include "gsm.h"
#include "app_config.h"

namespace Battery
{
    //
    // Private members
    //
    /** Battery gauge instance */
    MAX1704X _gauge(5);

    //
    // Private functions
    //
    uint8_t mv_to_pct(uint16_t mv);

    /******************************************************************************
     * Init fuel gauge
     ******************************************************************************/
    RetResult init()
    {
        // Go to sleep
        _gauge.sleep();
    }

    /******************************************************************************
     * Wake device up, read fuel gauge values and go back to sleep.
     * @param voltage Voltage output value
     * @param pct Percentage output value
     ******************************************************************************/
    RetResult read_gauge(int *voltage, int *pct)
    {
        delay(10);

        // Try to wake up from sleep
        int tries = 2;
        while(tries--)
        {
            if(!_gauge.isSleeping())
                break;

            _gauge.wake();
        }

        // If still didn't wake up, either stuck in sleep or communication failed
        if(_gauge.isSleeping())
        {
            Utils::serial_style(STYLE_RED);
            Serial.println(F("Could not communicate with fuel gauge."));
            Utils::serial_style(STYLE_RESET);
            return RET_ERROR;
        }

        // When waking up from sleep, if we don't wait enough time, the
        // previous values will be returned
        delay(600);

        // TODO: Convert to Volts
        *voltage = (int)(_gauge.voltage()*1000);
        *pct = (int)_gauge.percent();

        // Back to sleep
        _gauge.sleep();

        return RET_OK;
    }

    /******************************************************************************
     * Read battery measurements from GSM module.
     * GSM module must be ON
     ******************************************************************************/
    RetResult read_gsm(uint16_t *voltage, uint16_t *pct)
    {
        RetResult ret = RET_OK;

        if(GSM::get_battery_info(voltage, pct) != RET_OK)
        {
            Serial.println(F("Could not read battery info from GSM module."));
            return RET_ERROR;
        }

        return ret;
    }

    /******************************************************************************
     * Read battery mV with ADC (TCALL)
     * @param
     *****************************************************************************/
    RetResult read_adc(uint16_t *voltage, uint16_t *pct)
    {
       	uint32_t in = 0;
        for (int i = 0; i < ADC_BATTERY_LEVEL_SAMPLES; i++)
        {
            in += (uint32_t)analogRead(PIN_ADC_BAT);
        }
        in = (int)in / ADC_BATTERY_LEVEL_SAMPLES;

        uint16_t bat_mv = ((float)in / 4096) * 3600 * 2;

        *voltage = bat_mv;
        *pct = mv_to_pct(bat_mv);
               
        return RET_OK;
    }

    /******************************************************************************
     * Store battery gauge measurements in log
     *****************************************************************************/
    RetResult log_gauge()
    {
        int voltage = 0, pct = 0;

        if(read_gauge(&voltage, &pct) == RET_ERROR)
        {
            return RET_ERROR;
        }

        Log::log(Log::BATTERY, voltage, pct);

        Utils::serial_style(STYLE_BLUE);
        Serial.printf("Battery: %dmV | %d%% |\n", voltage, pct);
        Utils::serial_style(STYLE_RESET);

        return RET_OK;
    }

    /******************************************************************************
    * Store GSM module measurements in log
    ******************************************************************************/
    RetResult log_gsm()
    {
        uint16_t voltage = 0, pct = 0;

        if(read_gsm(&voltage, &pct) == RET_ERROR)
        {
            return RET_ERROR;
        }

        Log::log(Log::BATTERY_GSM, voltage, pct);

        Utils::serial_style(STYLE_BLUE);
        Serial.printf("GSM Battery: %dmV | %d%% |\n", voltage, pct);
        Utils::serial_style(STYLE_RESET);

        return RET_OK;
    }

    /******************************************************************************
    * Measure battery voltage from ADC and log
    ******************************************************************************/
    RetResult log_adc()
    {
        uint16_t voltage = 0;
        uint16_t pct = 0;

        if(read_adc(&voltage, &pct) == RET_ERROR)
        {
            return RET_ERROR;
        }

        Log::log(Log::BATTERY, voltage, pct);

        // No percent value, use -1
        Utils::serial_style(STYLE_BLUE);
        Serial.printf("ADC Battery: %dmV | %d%% |\n", voltage, pct);
        Utils::serial_style(STYLE_RESET);

        return RET_OK;
    }

    /******************************************************************************
     * Convert battery voltage to pct with the help of a look up table
     *****************************************************************************/
    uint8_t mv_to_pct(uint16_t mv)
    {
        const uint8_t array_size = sizeof(BATTERY_PCT_LUT) / sizeof(BATTERY_PCT_LUT[0]);
        uint8_t pct_out = 0;

        // mV larger than the maximum = 100%
        if(mv >= BATTERY_PCT_LUT[array_size - 1].mv)
        {
            pct_out = 100;
        }
        else
        {
            const BATTERY_PCT_LUT_ENTRY *cur_entry = nullptr;
            const BATTERY_PCT_LUT_ENTRY *prev_entry = &BATTERY_PCT_LUT[0];

            const BATTERY_PCT_LUT_ENTRY *next_entry = nullptr;

            for(int i = 0; i < array_size - 1; i++)
            {
                cur_entry = &(BATTERY_PCT_LUT[i]);
                next_entry = &(BATTERY_PCT_LUT[i+1]);
                
                if(mv > cur_entry->mv && mv < next_entry->mv)
                {
                    if(mv - cur_entry->mv < next_entry->mv - mv)
                    {
                        pct_out = cur_entry->pct;
                    }
                    else
                    {
                        pct_out = next_entry->pct;
                    }
                    break;
                }
                else if(mv == cur_entry->mv)
                {
                    pct_out = cur_entry->pct;
                    break;
                }
            }
        }

        return pct_out;
    }

    /******************************************************************************
     * Get battery mode depending on level
     *****************************************************************************/
    BATTERY_MODE get_current_mode()
    {
        uint16_t mv = 0, pct = 0;
        
        read_adc(&mv, &pct);

        if(pct > BATTERY_LEVEL_LOW)
        {
            return BATTERY_MODE::BATTERY_MODE_NORMAL;
        }
        else if(pct > BATTERY_LEVEL_SLEEP_CHARGE)
        {
            return BATTERY_MODE::BATTERY_MODE_LOW;
        }
        else
        {
            return BATTERY_MODE::BATTERY_MODE_SLEEP_CHARGE;
        }
    }

    /******************************************************************************
     * Print current battery mode to serial output
     *****************************************************************************/
    void print_mode()
    {
        Serial.print(F("Battery mode: "));
        switch (get_current_mode())
        {
        case BATTERY_MODE::BATTERY_MODE_LOW:
            Serial.println(F("Low"));
            break;
        case BATTERY_MODE::BATTERY_MODE_NORMAL:
            Serial.println(F("Normal"));
            break;
        case BATTERY_MODE::BATTERY_MODE_SLEEP_CHARGE:
            Serial.println(F("Sleep charge"));
            break;
        default:
            Utils::serial_style(STYLE_RED);
            Serial.println(F("Unknown"));
            Utils::serial_style(STYLE_RESET);
            break;
        }
    }
}