#include "Main.h"
#include "Cloak.h"
#include "Mark.h"

namespace Timers {

    TIMER Timers[] = {
        { Cloak::UpdateCloakClients, Cloak::TIMER_INTERVAL, 0 },
        { Mark::RotateClearNonExistingTargetIds, Mark::CLEAR_ROTATION_TIMER_INTERVAL, 0 },
        { SpawnProtection::UpdateSpawnProtectionValidity, SpawnProtection::TIMER_INTERVAL, 0 },
    };

    int __stdcall Update() {

        // call timers
        for (uint i = 0; (i < sizeof(Timers) / sizeof(TIMER)); i++) {
            if ((timeInMS() - Timers[i].tmLastCall) >= Timers[i].tmIntervallMS) {
                    Timers[i].tmLastCall = timeInMS();
                    Timers[i].proc();
            }
        }

        returncode = DEFAULT_RETURNCODE;
        return 0; // it doesnt matter what we return here since we have set the
                  // return code to "DEFAULT_RETURNCODE", so FLHook will just ignore
                  // it
    }
}
