#pragma once
#include <FLHook.h>

namespace Cloak
{
    void InitializeWithGameData();
    bool RegisterNoCloakSolar(const std::string& nickname, const uint objectId);
    bool RegisterObject(const uint objId);
    void SetTargetCloakState(const uint objId, const bool cloaked);
    bool IsFullyUncloaked(const uint objId);
    void __stdcall Elapse_Time_AFTER(float seconds);
    void GuidedInit(CGuided* guided, CGuided::CreateParms& parms);
    void __stdcall ShipAndSolarDestroyed(IObjRW* killedObj, bool killed, uint killerObjId);
    void __stdcall ShipAndSolarEquipDestroyed(IObjRW* obj, CEquip* equip, DamageEntry::SubObjFate fate, DamageList* dmgList);
    void __stdcall RequestCreateShip_After(unsigned int clientId);
    void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId);
    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet);
    bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet);
    int __cdecl Dock_Call(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response);
    void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId);
    void __stdcall GoTradelane(unsigned int clientId, struct XGoTradelane const& goToTradelane);
    void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
    void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
    void __stdcall ActivateEquip(unsigned int clientId, XActivateEquip const& activateEquip);
    void UserCmd_CLOAK(uint clientId, const std::wstring& wscParam);
    void UserCmd_UNCLOAK(uint clientId, const std::wstring& wscParam);
}
