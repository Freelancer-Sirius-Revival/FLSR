#pragma once
#include <FLHook.h>

namespace Carrier
{
    void InitializeWithGameData();
    void __stdcall LaunchComplete_After(unsigned int baseId, unsigned int shipId);
    void __stdcall JumpInComplete_After(unsigned int systemId, unsigned int shipId);
    int __cdecl Dock_Call(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response);
    void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId);
    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
    void __stdcall ReqShipArch_After(unsigned int archetypeId, unsigned int clientId);
    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    void UserCmd_Dock(const uint clientId, const std::wstring& wscParam);
}