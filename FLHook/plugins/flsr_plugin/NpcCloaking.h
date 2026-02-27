#pragma once
#include <FLHook.h>

namespace NpcCloaking
{
	void ReadFiles();
	void RegisterObject(uint objId);
	void SetTargetCloakState(const uint objId, const bool cloaked);
	void __stdcall Elapse_Time_AFTER(float seconds);
	void __stdcall ShipAndSolarDestroyed(IObjRW* killedObj, bool killed, uint killerObjId);
	void __stdcall ShipAndSolarEquipDestroyedHook(IObjRW* obj, CEquip* equip, DamageEntry::SubObjFate fate, DamageList* dmgList);
	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet);
	bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet);
}