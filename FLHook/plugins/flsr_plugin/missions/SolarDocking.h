#pragma once
#include <FLHook.h>

namespace Missions
{
    void RegisterDockableSolar(const uint objId, const uint baseId);

    namespace Hooks
    {
        namespace SolarDocking
        {
            bool __stdcall Send_FLPACKET_SERVER_LAUNCH(uint iClientID, FLPACKET_LAUNCH& pLaunch);
            void __stdcall PlayerLaunch(unsigned int objId, unsigned int clientId);
            void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
            int __cdecl Dock_Call_After(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, DOCK_HOST_RESPONSE response);
            void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
        }
    }
}