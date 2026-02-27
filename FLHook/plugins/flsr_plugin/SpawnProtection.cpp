#include "main.h"

namespace SpawnProtection
{
    const mstime TIMER_INTERVAL = 500;
    std::unordered_map<uint, mstime> protectionEndTimePerShip;
    mstime protectionDuration = 0;

    void LoadSettings()
    {
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + "\\flhook_plugins\\flsr.cfg";
        protectionDuration = IniGetI(scPluginCfgFile, "SpawnProtection", "ProtectionDuration", 0) * 1000;
	}

    static bool IsInNoPvpSystem(const uint shipId)
    {
        uint systemId;
        pub::SpaceObj::GetSystem(shipId, systemId);
        for (const auto& noPvPSystem : map_mapNoPVPSystems)
        {
            if (noPvPSystem.second == systemId)
                return true;
        }
        return false;
    }

    static void ActivateSpawnProtection(const uint shipId)
    {
        pub::SpaceObj::SetInvincible2(shipId, true, true, 0);
        protectionEndTimePerShip[shipId] = timeInMS() + protectionDuration;
    }

    void UpdateSpawnProtectionValidity()
    {
        const mstime now = timeInMS();
        auto it = protectionEndTimePerShip.begin();
        while (it != protectionEndTimePerShip.end())
        {
            // Protection time ended
            const mstime protectionStartTime = it->second;
            if (now > protectionStartTime)
            {
                const uint shipId = it->first;
                if (pub::SpaceObj::ExistsAndAlive(shipId) == 0) // 0 -> true
                {
                    if (IsInNoPvpSystem(shipId))
                        pub::SpaceObj::SetInvincible2(shipId, false, true, 0);
                    else
                        pub::SpaceObj::SetInvincible2(shipId, false, false, 0);
                }
                it = protectionEndTimePerShip.erase(it);
            }
            else
                it++;
        }
    }

    void __stdcall SystemSwitchOutComplete_AFTER(unsigned int shipId, unsigned int clientId)
    {
        ActivateSpawnProtection(shipId);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall PlayerLaunch_AFTER(unsigned int shipId, unsigned int clientId)
    {
        ActivateSpawnProtection(shipId);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall LaunchComplete_AFTER(unsigned int baseId, unsigned int shipId)
    {
        ActivateSpawnProtection(shipId);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall JumpInComplete_AFTER(unsigned int systemId, unsigned int shipId)
    {
        ActivateSpawnProtection(shipId);
        returncode = DEFAULT_RETURNCODE;
    }
}