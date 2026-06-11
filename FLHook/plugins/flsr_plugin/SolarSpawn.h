#pragma once
#include <FLHook.h>

namespace SolarSpawn
{
    struct SolarArchetype
    {
        bool autospawn = false;
        uint archetypeId = 0;
        uint loadoutId = 0;
        std::string nickname = "";
        uint nicknameCounter = 0;
        uint idsName = 0;
        uint idsInfocard = 0;
        Vector position;
        Matrix orientation;
        uint systemId = 0;
        uint baseId = 0;
        std::string affiliation = "";
        uint pilotId = 0;
        int hitpoints = -1;
        uint voiceId = 0;
        uint headId = 0;
        uint bodyId = 0;
        std::vector<uint> accessoryIds = {};
    };

    void LoadSettings();
    void Initialize();
    void AppendSolarArchetype(const SolarArchetype& archetype);
    bool __stdcall Send_FLPACKET_SERVER_LAUNCH(uint iClientID, FLPACKET_LAUNCH& pLaunch);
    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
    int __cdecl Dock_Call_After(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, DOCK_HOST_RESPONSE response);
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
    uint SpawnSolarByName(std::string name);
    bool DestroySolar(const uint spaceObjId, const DestroyType destroyType);
}
