#include <HardwareSerial.h>
#include <esp_sleep.h>
#include "sleep.h"
#include "const.h"
#include "app_config.h"
#include "struct.h"
#include "log.h"
#include "device_config.h"

namespace Sleep
{
	//
	// Private vars
	//

	/** Internal time keeping. Used to calculate next wake up time */
	uint32_t _last_t = 0;

	/** millis() of last time device went to sleep  */
	uint32_t _last_sleep_ms = 0;

	/** Reasons of last wake up event */
	int _last_wakeup_reasons = 0;

	//
	// Private functions
	//
	RetResult calc_next_sleep_mins();
	Sleep::WakeupScheduleEntry* get_schedule();

	/******************************************************************************
	* Decide minutes to sleep and go to sleep
	******************************************************************************/
	RetResult sleep()
	{
		// Calc how many mins are left until the next wake up event and what are the reasons
		// Do the calculation keeping own time (_last_t) and not actual time and each time add calculated
		// time to current time.
		// This way no wake up events are actually lost when an event takes long enough time to
		// overlap into the next wake up event.

		// TODO: 
		// A possibly better approach would be to use millis():
		// 1. Keep millis() when first_sleep occurred and use as 0
		// 2. After each wake up, keep wake up millis()
		// 3. When calculating sleep, use millis since first_sleep in calc_next_wakeup
		// 4. Find all wake up reasons that would have occurred while we were busy working and set flags
		// This way no matter how much an event is overdue, all the missed events will be executed once
		// (no matter how many times they were skipped) and the schedule will continue as usual

		// TODO: Internal timer overlap

		//
		// Prepare for sleep
		//
		int sleep_time_mins = 0;

		WakeupScheduleEntry *schedule = get_schedule();
		
		calc_next_wakeup(_last_t, schedule, WAKEUP_SCHEDULE_LEN, &sleep_time_mins, &_last_wakeup_reasons);		

		_last_t += sleep_time_mins;

		Serial.print(F("Sleeping for minutes: "));
		Serial.println(sleep_time_mins);

		// How long are we awake? Subtract this time from time to sleep to correct it
		// uint32_t time_awake_mins = round((millis() - _last_sleep_ms) / 60000);
		// int mins_to_sleep = sleep_time_mins - time_awake_mins;
		// uint32_t time_awake_sec = (millis() - _last_sleep_ms) / 1000;
		// int time_to_sleep_sec = (sleep_time_mins * 60) - time_awake_ms * 

		// Time since last sleep, Used to calculate correction for next wakeup
		uint32_t time_awake_ms = 0;
		if(_last_sleep_ms != 0)
			time_awake_ms = millis() - _last_sleep_ms;

		// Time to sleep in ms, minus time since last sleep
		int time_to_sleep_ms = (sleep_time_mins * 60000) - time_awake_ms;

		Serial.print(F("Time awake (sec): "));
		Serial.println(time_awake_ms / 1000, DEC);
		
		if(time_to_sleep_ms < 0)
		{
			Serial.printf("Last process was overdue by secs: %d\n", time_to_sleep_ms / 1000);
			time_to_sleep_ms = 0;

			Serial.println(F("Skipping sleep."));

			// Keep sleep time
			_last_sleep_ms = millis();
		}
		else
		{			
			//
			// Sleep
			//
			// When testing treat minutes as seconds
			if(SLEEP_MINS_AS_SECS)
			 	time_to_sleep_ms /= 60;

			// Log sleep
			Log::log(Log::SLEEP, time_awake_ms / 1000);

			Serial.print(F("Sleeping for (s): "));
			Serial.println(time_to_sleep_ms / 1000, DEC);
			Serial.flush();
			esp_sleep_enable_timer_wakeup(time_to_sleep_ms * 1000);
			esp_light_sleep_start();

			//
			// Wake up!
			//
			Log::log(Log::WAKEUP);
			Serial.println(F("Wake up!"));

			// Keep sleep time
			// TODO: this is actually time away
			_last_sleep_ms = millis();
		}

		return RET_OK;
	}

	/******************************************************************************
	* Calculate minutes left until next wake up event in the schedule, if current
	* time is t.
	* @param t          Current time
	* @schedule         Wakeup events
	* @schedule_len     Size of wake up schedule array
	* @sleep_mins_out   Minutes to next event (output var)
	* @next_reasons_out Reasons of next event (output var)
	* All params exposed for Testing
	******************************************************************************/
	RetResult calc_next_wakeup(int t, const WakeupScheduleEntry schedule[], int schedule_len,
		int *sleep_mins_out, int *next_reasons_out)
	{
		// TODO: Must check for s[] being 0 (on boot test) else we have 
		// division by zero
		// TODO: how does this behave when values in schedule are not ordered and unique?

		// Minutes to next wake up event
		int next_wakeup_mins = 0;

		// Interval of next wakeup event, used to find all reasons that occur at the same time
		int next_wakeup_interval = 0;

		// Iterate all wake up intervals and see which is closest to this point in time
		for(int i = 0; i < schedule_len; i++)
		{
			Serial.print(F("Reason: "));
			Serial.println(schedule[i].reason, DEC);
			Serial.print(F("Interval: "));
			Serial.println(schedule[i].interval_mins, DEC);

			int diff_to_next = 0;
			const int cur_interval_mins = schedule[i].interval_mins;

			diff_to_next = ((int)(t / cur_interval_mins) + 1) * cur_interval_mins;
			diff_to_next = diff_to_next - t;

			// Min difference to next wake up time
			if(diff_to_next < next_wakeup_mins || next_wakeup_mins == 0)
			{
				next_wakeup_mins = diff_to_next;
				next_wakeup_interval = cur_interval_mins;
			}
		}

		// Find reasons that occur at the same wake up time and OR
		int reasons = 0;

		for(int i = 0; i < schedule_len; i++)
		{
			if((t + next_wakeup_mins) % schedule[i].interval_mins == 0)
			{
				reasons = reasons | schedule[i].reason;
			}
		}

		// Output results
		*sleep_mins_out = next_wakeup_mins;
		*next_reasons_out = reasons;

		return RET_OK;
	}

	/******************************************************************************
	 * Decide which wake up schedule to use and return
	 *****************************************************************************/
	Sleep::WakeupScheduleEntry* get_schedule()
	{
		return (WakeupScheduleEntry*)&WAKEUP_SCHEDULE_DEFAULT;
	}

	/******************************************************************************
	* Check if one of the reasons of last wake up is "reason" (multiple reasons
	* cam be)
	******************************************************************************/
	bool wakeup_reason_is(WakeupReason reason)
	{
		// TODO: For now returns only the default schedule
		// When the wakeup schedule becomes a fixed size struct, it must be updated to return 
		// the correct schedule depending on conditions (battery level)

		return _last_wakeup_reasons & reason ? true : false;
	}
}