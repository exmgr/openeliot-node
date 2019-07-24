#ifndef STRUCT_H
#define STRUCT_H

#include <inttypes.h>

/**
 * Device descriptor
 * Used for device-specific configuration
 */
struct DeviceDescriptor
{
    // Simple device id
    uint8_t id;

    // Mac address
    char mac[18];

    // APN used by this device. Useful for when debug device uses different SIM card
    // (enough memory to waste on ESP32)
    char apn[20];

    // Thingsboard access token
    char tb_access_token[21];
}__attribute__((packed));

/**
 * Console styles
 */
enum SerialStyle
{
    STYLE_RESET = 0, STYLE_BOLD, STYLE_FAINT,

    STYLE_BLACK = 30, STYLE_RED, STYLE_GREEN, STYLE_YELLOW, STYLE_BLUE,
    STYLE_MAGENTA, STYLE_CYAN, STYLE_WHITE,

    STYLE_BLACK_BKG = 40, STYLE_RED_BKG, STYLE_GREEN_BKG, STYLE_YELLOW_BKG,
    STYLE_BLUE_BKG, STYLE_MAGENTA_BKG, STYLE_CYAN_BKG, STYLE_WHITE_BKG
};

/**
 * Generic function result
 */
enum RetResult
{
    // Generic OK code
    RET_OK = 0,
    // Generic failure code
    RET_ERROR = 1
};

/**
 * Water Level sensor input channels
 */
enum WaterLevelChannel
{
    WATER_LEVEL_CHANNEL_PWM = 1,
    WATER_LEVEL_CHANNEL_ANALOG,
    WATER_LEVEL_CHANNEL_SERIAL
};


#endif