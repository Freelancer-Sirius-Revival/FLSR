#pragma once
#include <FLHook.h>

namespace Mark
{
    extern const uint CLEAR_ROTATION_TIMER_INTERVAL;
    bool MarkObject(const uint clientId, const uint targetId);
    bool UnmarkObject(const uint clientId, const uint targetId);
    void AddCloakedPlayer(const uint clientId);
    void RemoveCloakedPlayer(const uint clientId);
    void RotateClearNonExistingTargetIds();
    void UserCmd_Mark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_GroupMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnGroupMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnMarkAll(const uint clientId, const std::wstring& wscParam);
    void __stdcall SystemSwitchOutComplete_After(unsigned int shipId, unsigned int clientId);
    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state);
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
}