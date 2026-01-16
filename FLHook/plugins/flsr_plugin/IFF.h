#pragma once
#include <FLHook.h>

namespace IFF
{
    void ReadCharacterData();
    void UserCmd_Hostile(const uint clientId, const std::wstring& arguments);
    void UserCmd_Neutral(const uint clientId, const std::wstring& arguments);
    void UserCmd_Allied(const uint clientId, const std::wstring& arguments);
    void UserCmd_Attitude(const uint clientId, const std::wstring& arguments);
    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship);
    void __stdcall ShipEquipDamage(const IObjRW* damagedObject, const CEquip* hitEquip, const float& incomingDamage, const DamageList* damageList);
    void __stdcall ShipShieldDamage(const IObjRW* damagedObject, const CEShield* hitShield, const float& incomingDamage, const DamageList* damageList);
    void __stdcall ShipColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList);
    void __stdcall ShipHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList);
    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
}