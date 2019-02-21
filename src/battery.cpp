#include "battery.h"
#include "MAX17043.h"
#include "const.h"
#include "log.h"
#include "utils.h"
#include "gsm.h"

namespace Battery
{
    MAX1704X gauge(5);

    /******************************************************************************
     * Init fuel gauge
     ******************************************************************************/
    RetResult init()
    {
        // Go to sleep
        gauge.sleep();
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
            if(!gauge.isSleeping())
                break;

            gauge.wake();
        }

        // If still didn't wake up, either stuck in sleep or communication failed
        if(gauge.isSleeping())
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
        *voltage = (int)(gauge.voltage()*1000);
        *pct = (int)gauge.percent();

        // Back to sleep
        gauge.sleep();

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
     * Read battery measurements from GSM module.
     * GSM module must be ON
     ******************************************************************************/
    RetResult read_gsm(uint16_t *voltage, uint16_t *pct)
    {
        RetResult ret = RET_OK;

        if(!GSM::get_battery_info(voltage, pct))
        {
            Serial.println(F("Could not read battery info from GSM module."));
            return RET_ERROR;
        }

        return ret;
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
    
}