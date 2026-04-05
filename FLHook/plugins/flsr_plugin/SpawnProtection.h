#pragma once

namespace SpawnProtection
{
    extern const mstime TIMER_INTERVAL;
    void LoadSettings();
    void UpdateSpawnProtectionValidity();
    void __stdcall SystemSwitchOutComplete_AFTER(unsigned int shipId, unsigned int clientId);
    void __stdcall PlayerLaunch_AFTER(unsigned int shipId, unsigned int clientId);
    void __stdcall LaunchComplete_AFTER(unsigned int baseId, unsigned int shipId);
    void __stdcall JumpInComplete_AFTER(unsigned int systemId, unsigned int shipId);
}