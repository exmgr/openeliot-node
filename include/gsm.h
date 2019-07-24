#ifndef _GSM_H
#define _GSM_H

#include "Adafruit_FONA.h"
#include "struct.h"
#include "app_config.h"
#include "const.h"
#include <TinyGsmClient.h>

namespace GSM
{
    extern Adafruit_FONA_LTE fona;

    void init();

    RetResult connect();
    RetResult connect_persist();

    RetResult enable_gprs(bool enable);

    RetResult update_ntp_time();
    RetResult get_time(tm *out);

    RetResult on();
    RetResult off();

    RetResult get_battery_info(uint16_t *voltage, uint16_t *pct);

    TinyGsm* get_modem();

    int get_rssi();
    bool sim_card_present();
}

#endif