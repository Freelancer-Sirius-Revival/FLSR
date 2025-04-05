#pragma once
#include <FLHook.h>

namespace Missions
{
    void Initialize();
    void __stdcall Shutdown();
    void StartMissionByOfferId(const uint offerId, const uint clientId);
    void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
    void __stdcall Elapse_Time_AFTER(float seconds);
    void __stdcall CharacterSelect(const CHARACTER_ID& cId, unsigned int clientId);
    void __stdcall CharacterSelect_AFTER(const CHARACTER_ID& cId, unsigned int clientId);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2);
    void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
    void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
}