#pragma once
#include <FLHook.h>
#include "Mission.h"

namespace Missions
{
    void StartMissionByOfferId(const uint offerId, const uint startingClientId, const std::vector<uint>& clientIds);
    bool IsPartOfOfferedJob(const uint clientId);
    void RemoveClientFromCurrentOfferedJob(const uint clientId);

    void Initialize();
    void __stdcall Shutdown();
    void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
    void __stdcall CharacterSelect(const CHARACTER_ID& cId, unsigned int clientId);
    void __stdcall CharacterSelect_AFTER(const CHARACTER_ID& cId, unsigned int clientId);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2);
    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
}