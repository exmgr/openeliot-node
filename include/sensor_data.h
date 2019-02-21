#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include "app_config.h"
#include "struct.h"
#include "data_store.h"

namespace SensorData
{
    /**
     * Water sensor data packet
     * All ranges/resolutions are for Aquatroll400
     * NOTE: MUST be aligned to 4 byte boundary to avoid padding. If not, CRC32 calculations
     * may fail
     */
    struct Entry
    {
        uint32_t timestamp;

        // Range: -5 - 50C 0.01C
        float temperature;

        // Range: 0-60mg/L / Res: 0.01mg/L
        float dissolved_oxygen;

        // Range: 0-100mS/cm - Res: 0.1uS/cm
        float conductivity;

        // Range: 0-14 / Res 0.01
        float ph;

        // Range: 20 - 1064cm / Res: ?
        int water_level;
    }__attribute__((packed));

    bool add(Entry *data);

    DataStore<Entry>* get_store();

    void print(const Entry *data);
}

#endif