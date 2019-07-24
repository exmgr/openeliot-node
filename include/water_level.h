#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H
#include "sensor_data.h"

namespace WaterLevel
{
	RetResult init();

	RetResult measure(SensorData::Entry *data);

	RetResult measure_dummy(SensorData::Entry *data);
}

#endif