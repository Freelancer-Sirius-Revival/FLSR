#include "main.h"

namespace SpawnProtection
{
    const mstime TIMER_INTERVAL = 500;
    static std::map<uint, mstime> invincibleEndTimePerShip;
    static mstime invincibleDuration = 0;

    bool LoadSettings()
    {
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + Globals::PLUGIN_CONFIG_FILE;

        invincibleDuration = IniGetI(scPluginCfgFile, "SpawnProtection", "ProtectionDuration", 0) * 1000;
        return true;
	}

    void SetInvincible(const uint shipId)
    {
        pub::SpaceObj::SetInvincible(shipId, true, true, 0);
        invincibleEndTimePerShip[shipId] = timeInMS() + invincibleDuration;
    }

    void UpdateInvincibleStates()
    {
        if (Modules::GetModuleState("SpawnProtection"))
        {
            const mstime now = timeInMS();
            std::vector<uint> shipIdsToDelete;
            for (const auto& invincibleShipTime : invincibleEndTimePerShip)
            {
                if (now > invincibleShipTime.second)
                {
                    if (pub::SpaceObj::ExistsAndAlive(invincibleShipTime.first) == 0) // 0 -> true
                        pub::SpaceObj::SetInvincible(invincibleShipTime.first, false, false, 0);
                    shipIdsToDelete.push_back(invincibleShipTime.first);
                }
            }
            for (const uint shipId : shipIdsToDelete)
                invincibleEndTimePerShip.erase(shipId);
        }
    }

    void __stdcall SystemSwitchOutComplete_AFTER(unsigned int shipId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (Modules::GetModuleState("SpawnProtection"))
            SetInvincible(shipId);
    }

    void __stdcall PlayerLaunch_AFTER(unsigned int shipId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (Modules::GetModuleState("SpawnProtection"))
            SetInvincible(shipId);
    }

    void __stdcall LaunchComplete_AFTER(unsigned int baseId, unsigned int shipId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (Modules::GetModuleState("SpawnProtection"))
            SetInvincible(shipId);
    }

    void __stdcall JumpInComplete_AFTER(unsigned int systemId, unsigned int shipId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (Modules::GetModuleState("SpawnProtection"))
            SetInvincible(shipId);
    }

    bool AllowPlayerDamage(uint clientId, uint targetClientId)
    {
        returncode = DEFAULT_RETURNCODE;

        bool preventNpcDamage;
        bool preventPlayerDamage;
        float threshold;
        uint shipId;
        pub::Player::GetShip(targetClientId, shipId);
        if (shipId)
        {
            pub::SpaceObj::GetInvincible(shipId, preventNpcDamage, preventPlayerDamage, threshold);
            if (preventPlayerDamage)
            {
                returncode = SKIPPLUGINS_NOFUNCTIONCALL;
                return false;
            }
        }

        return true;
    }
}