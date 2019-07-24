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

        /** Set after OTA to denote that a new FW was flashed, so the new FW knows its new
         * Must be cleared on boot. */
        bool ota_flashed;

        /** Data id of last remote control data applied. Used to check if received config data is new */
        unsigned int last_rc_data_id;
    }__attribute__((packed));

    RetResult init();
    const Data* get();
    RetResult commit();
    void print(const Data *data);
    void print_current();

    // Getters
    int get_last_rc_data_id();
    
    // Setters
    RetResult set_clean_reboot(bool val);
    RetResult set_wakeup_schedule(const Sleep::WakeupScheduleEntry* schedule);
    RetResult set_wakeup_schedule_reason_int(Sleep::WakeupReason reason, int int_mins);
    RetResult set_remote_control_data(const RemoteControl::Data *new_data);
    RetResult set_last_rc_data_id(int id);
    RetResult set_ota_flashed(bool val);

    int get_wakeup_schedule_reason_int(Sleep::WakeupReason reason);
    bool get_clean_reboot();
    bool get_ota_flashed();
}

#endif