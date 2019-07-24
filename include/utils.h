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

    RetResult ip5306_set_power_boost_state(bool enable);

    RetResult url_explode(char *in, int *port_out, char *host_out, int host_max_size, char *path_out, int path_max_size);

    RetResult tb_build_attributes_url_path(char *buff, int buff_size);

    RetResult tb_build_telemetry_url_path(char *buff, int buff_size);

    RetResult restart_device();

    void print_reset_reason();
}

#endif