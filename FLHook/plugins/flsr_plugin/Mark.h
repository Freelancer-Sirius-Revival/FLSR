#pragma once
#include <FLHook.h>

namespace Mark
{
    extern const uint CLEAR_ROTATION_TIMER_INTERVAL;
    bool Mark(const uint clientId, const uint targetId);
    bool Unmark(const uint clientId, const uint targetId);
    void AddInvisibleObject(const uint objectId);
    void RemoveInvisibleObject(const uint objectId);
    void UserCmd_Mark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_GroupMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnGroupMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnMarkAll(const uint clientId, const std::wstring& wscParam);
    void __stdcall SystemSwitchOutComplete_After(unsigned int shipId, unsigned int clientId);
    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship);
    bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& solar);
    bool __stdcall Send_FLPACKET_SERVER_CREATELOOT_AFTER(uint clientId, FLPACKET_CREATELOOT& loot);
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall LootDestroyed(const IObjRW* killedObject);
    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
}