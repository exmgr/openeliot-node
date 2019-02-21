#ifndef CONFIG_H
#define CONFIG_H

#include <inttypes.h>
#include <Preferences.h>
#include "remote_control.h"
#include "struct.h"
#include "utils.h"
#include "log.h"

namespace DeviceConfig
{
    struct Data
    {
        /** CRC32 of whole structure. Calculated with crc32 = 0 */
        uint32_t crc32;

        /** Set before doing a reboot. If not set during boot, reboot was unexpected. */
        bool clean_reboot;
       
        /** Current wake up schedule as set by user or loaded from defaults */
        Sleep::WakeupScheduleEntry wakeup_schedule[WAKEUP_SCHEDULE_LEN];

        /** When set, there is config from remote_control_data that needs to be applied.
         * Set when new remote control data is downloaded and there is config that can
         * only be applied on boot */
        bool apply_remote_control_on_boot;

        /** Last received remote control data. Also contains the config id from which is
         * decided if the config received is new or already applied. */
        RemoteControl::Data remote_control_data;
    }__attribute__((packed));

    RetResult init();
    const Data* get();
    RetResult commit();
    void print(const Data *data);
    void print_current();

    // Getters
    const RemoteControl::Data* get_remote_control_data();

    // Setters
    RetResult set_clean_reboot(bool val);
    RetResult set_apply_remote_control(bool val);
    RetResult set_wakeup_schedule(const Sleep::WakeupScheduleEntry* schedule);
    RetResult set_wakeup_schedule_reason_int(Sleep::WakeupReason reason, int int_mins);
    RetResult set_remote_control_data(const RemoteControl::Data *new_data);

    int get_wakeup_schedule_reason_int(Sleep::WakeupReason reason);
    bool get_clean_reboot();
}

#endif