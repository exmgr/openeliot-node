#ifndef SLEEP_H
#define SLEEP_H

#include "struct.h"

namespace Sleep
{
    // Reason that triggered wake up
    enum WakeupReason
    {
        REASON_NONE = 0,
        REASON_READ_WATER_SENSORS = 1,
        REASON_CALL_HOME = 1 << 1
    };

    // Describes an entry in the wake up schedule
    // "Wakeup" and not "Sleep" because it represents wake up intervals and
    // not sleep intervals (ie. how often it wakes up, not how often it sleeps)
    struct WakeupScheduleEntry
    {
        WakeupScheduleEntry(){}
        WakeupScheduleEntry(WakeupReason _reason, int _interval_mins)
            : reason(_reason), interval_mins(_interval_mins)
        {}

        // Reason that triggered wake up
        WakeupReason reason;
        // Wake up interval in minutes
        int interval_mins;
    }__attribute__((packed));

    RetResult sleep();

    RetResult calc_next_wakeup(int t, const WakeupScheduleEntry schedule[], int schedule_len,
        int *sleep_mins_out, int *next_reasons_out);

    bool wakeup_reason_is(WakeupReason reason);
    void print_schedule(Sleep::WakeupScheduleEntry schedule[]);
}

#endif