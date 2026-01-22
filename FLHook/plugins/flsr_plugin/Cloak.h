#pragma once
#include <FLHook.h>

namespace Cloak
{
    enum class CloakState
    {
        Uncloaked,
        Cloaking,
        Cloaked,
        Uncloaking
    };

    void InitializeWithGameData();
    void UpdateCloakClients();
    CloakState GetClientCloakState(uint clientId);
    bool TryRegisterNoCloakSolar(const std::string& nickname, uint objectId);
    extern const uint TIMER_INTERVAL;
    void __stdcall ActivateEquip(unsigned int clientId, XActivateEquip const& activateEquip);
    void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId);
    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
    void __stdcall GoTradelane(unsigned int clientId, struct XGoTradelane const& goToTradelane);
    int __cdecl Dock_Call(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response);
    void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId);
    void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
    void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
    void __stdcall SPObjUpdate(SSPObjUpdateInfo const& updateInfo, unsigned int clientId);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state);
    void __stdcall ShipEquipDestroyed(const IObjRW* object, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* damageList);
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void GuidedInit(CGuided* guided, CGuided::CreateParms& parms);
    void __stdcall ActivateCruise(unsigned int clientId, struct XActivateCruise const& activateCruise);
    void UserCmd_CLOAK(uint clientId, const std::wstring& wscParam);
    void UserCmd_UNCLOAK(uint clientId, const std::wstring& wscParam);
}