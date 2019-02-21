#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <inttypes.h>
#include "struct.h"

namespace Utils
{
    void get_mac(char *out, uint8_t size);
    const DeviceDescriptor* get_device_descriptor();

    void print_separator(const __FlashStringHelper *name);

    void print_block(const __FlashStringHelper *title);

    void serial_style(SerialStyle style);

    uint32_t crc32(uint8_t *buff, uint32_t buff_size);

    int battery_level();
}

#endif