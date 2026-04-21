#pragma once
#include <FLHook.h>

namespace CounterMeasuresRecharge
{
	void __stdcall FireWeapon(uint clientId, const XFireWeaponInfo& weapon);
	void __stdcall ActivateCruise(unsigned int clientId, const XActivateCruise& activateCruise);
	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId);
	void __stdcall ShipEquipDestroyed(const IObjRW* object, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* damageList);
	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
	void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
	void __stdcall Elapse_Time_After(float seconds);
}