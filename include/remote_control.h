#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#include "struct.h"
#include "const.h"

namespace RemoteControl
{
	struct Data
	{
		Data(int _water_sensors_measure_int_mins, int _call_home_int_mins,
		bool _reboot, bool _ota)
		:
		water_sensors_measure_int_mins(_water_sensors_measure_int_mins),
		call_home_int_mins(_call_home_int_mins),
		reboot(_reboot),
		ota(_ota)
		{}

		Data(){}

		int id;
		int water_sensors_measure_int_mins;
		int call_home_int_mins;
		bool reboot;
		bool ota;
	}__attribute__((packed));

	RetResult start();

	bool get_reboot_pending();
	void set_reboot_pending(bool val);

	void print(const Data *data);
}

#endif