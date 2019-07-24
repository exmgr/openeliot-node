#ifndef WATER_QUALITY_H
#define WATER_QUALITY_H

#include "sensor_data.h"

namespace WaterQualitySensor
{
    RetResult init();

    RetResult measure(SensorData::Entry *data);

    RetResult measure_dummy(SensorData::Entry *data);
}

#endif