#ifndef BATTERY_H
#define BATTERY_H
#include "struct.h"
#include "const.h"

namespace Battery
{
    RetResult init();
    RetResult read_gauge(int *voltage, int *pct);
    RetResult log_gauge();
    RetResult read_gsm(uint16_t *voltage, uint16_t *pct);
    RetResult log_gsm();
    RetResult read_adc(uint16_t *voltage, uint16_t *pct);
    RetResult log_adc();
    BATTERY_MODE get_current_mode();
    void print_mode();
}

#endif