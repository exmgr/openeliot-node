#ifndef WIFI_MODEM_H
#define WIFI_MODEM_H

#if WIFI_DATA_SUBMISSION || WIFI_DEBUG_SERIAL

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "struct.h"

namespace WifiModem
{
    void init();

    RetResult connect();
    RetResult disconnect();

    bool is_connected();
}

#endif

#endif