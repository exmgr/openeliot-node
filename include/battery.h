#ifndef BATTERY_H
#define BATTERY_H
#include "struct.h"

namespace Battery
{
    RetResult init();
    RetResult read_gauge(int *voltage, int *pct);
    RetResult log_gauge();
    RetResult read_gsm(uint16_t *voltage, uint16_t *pct);
    RetResult log_gsm();
}

#endif