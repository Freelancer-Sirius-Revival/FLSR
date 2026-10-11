#include "ShieldSync.h"
#include "../plugin.h"

namespace ShieldSync
{
	static void SendEquipmentActivationState(const uint clientId, const uint objId, const uint subObjId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.iSpaceID = objId;
		activateEquipment.sID = subObjId;
		activateEquipment.bActivate = active;
		GetClientInterface()->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	static void SendShieldState(const uint clientId, const uint objId)
	{
		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return;
		const auto& shield = dynamic_cast<CEqObj*>(inspect->cobj)->equip_manager.FindFirst(EquipmentClass::Shield);
		if (!shield)
			return;

		// Freelancer fails to send the deactivate-state of shields upon object creation.
		if (!shield->IsActive())
		{
			XActivateEquip activateEquipment;
			activateEquipment.iSpaceID = objId;
			activateEquipment.sID = shield->iSubObjId;
			activateEquipment.bActivate = shield->IsActive();
			GetClientInterface()->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
		}

		// Freelancer fails to send the shield hitpoints upon object creation. If there is no regeneration rate, they will stay full forever for the client.
		if (shield->GetHitPoints() != shield->GetMaxHitPoints())
		{
			DamageList dmg;
			dmg.add_damage_entry(shield->iSubObjId, shield->GetHitPoints(), DamageEntry::SubObjFate(0));
			GetClientInterface()->Send_FLPACKET_SERVER_DAMAGEOBJECT(clientId, objId, dmg);
		}
	}

	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet)
	{
		SendShieldState(clientId, packet.iSpaceID);
		returncode = DEFAULT_RETURNCODE;
		return true;
	}

	bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet)
	{
		SendShieldState(clientId, packet.iSpaceID);
		returncode = DEFAULT_RETURNCODE;
		return true;
	}
}