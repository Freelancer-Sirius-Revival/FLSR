#include "CloakSync.h"
#include "plugin.h"

namespace CloakSync
{
	static void SendEquipmentActivationState(const uint clientId, const uint otherObjId, const uint subObjId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = otherObjId;
		activateEquipment.sID = subObjId;
		GetClientInterface()->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	static void SyncCloakingDeviceState(const uint clientId, uint objId, const EquipDescVector& equipList)
	{
		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return;
		const auto& object = dynamic_cast<CEqObj*>(inspect->cobj);

		for (const auto& entry : equipList.equip)
		{
			if (!entry.bMounted)
				continue;

			const auto& equip = object->equip_manager.FindByID(entry.sID);
			if (equip->CEquipType == EquipmentClass::CloakingDevice)
				SendEquipmentActivationState(clientId, objId, entry.sID, equip->isActive);
		}
	}

	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet)
	{
		returncode = DEFAULT_RETURNCODE;
		SyncCloakingDeviceState(clientId, packet.iSpaceID, packet.equip);
		return true;
	}

	bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet)
	{
		returncode = DEFAULT_RETURNCODE;
		SyncCloakingDeviceState(clientId, packet.iSpaceID, packet.equip);
		return true;
	}
}