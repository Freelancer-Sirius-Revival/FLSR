#include "Main.h"

namespace Timers {

    TIMER Timers[] = {
        { Cloak::UpdateCloakClients, Cloak::TIMER_INTERVAL, 0 },
        { Mark::RotateClearNonExistingTargetIds, Mark::CLEAR_ROTATION_TIMER_INTERVAL, 0 },
        { EquipWhiteList::ProcessChangedEquipments, EquipWhiteList::TIMER_INTERVAL, 0 },
        { SpawnProtection::UpdateSpawnProtectionValidity, SpawnProtection::TIMER_INTERVAL, 0 },
       {Tools::CharSelectMenu, 2000, 0},
    };

    int __stdcall Update() {
        returncode = DEFAULT_RETURNCODE;

        // call timers
        for (uint i = 0; (i < sizeof(Timers) / sizeof(TIMER)); i++) {
            if ((timeInMS() - Timers[i].tmLastCall) >= Timers[i].tmIntervallMS) {
                    Timers[i].tmLastCall = timeInMS();
                    Timers[i].proc();
            }
        }

        return 0; // it doesnt matter what we return here since we have set the
                  // return code to "DEFAULT_RETURNCODE", so FLHook will just ignore
                  // it
    }
}
