#ifndef _GSM_H
#define _GSM_H

#include "Adafruit_FONA.h"
#include "struct.h"

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

    RetResult req(const char *url, const char *method, const char *req_data, int req_data_size, 
            char *resp_buffer = NULL, int resp_buffer_size = 0);

    RetResult get_req(char *url, uint16_t *status_code, uint16_t *resp_len, char *resp_buff, int resp_buff_size);

    int get_rssi();
    bool sim_card_present();
}

#endif