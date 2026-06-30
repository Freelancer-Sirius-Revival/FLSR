#pragma once
#include <FLHook.h>

namespace CargoPods
{
	void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
	void __stdcall ShipEquipDestroyed(const IObjRW* iobj, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* dmgList);
}