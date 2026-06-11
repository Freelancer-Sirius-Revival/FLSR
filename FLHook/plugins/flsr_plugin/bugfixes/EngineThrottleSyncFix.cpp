#include "EngineThrottleSyncFix.h"
#include "../Plugin.h"

namespace EngineThrottleSyncFix
{
	/* Make sure to set the engine throttle to zero when engine kill happens. Or the server keeps drawing power from the engine. */
	void __stdcall ActivateEquip(unsigned int clientId, const XActivateEquip& activateEquip)
	{
		// Only catch the case when engine gets deactivated.
		if (activateEquip.bActivate)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(activateEquip.iSpaceID, inspect, system) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		const auto& equip = reinterpret_cast<CEqObj*>(inspect->cobj)->equip_manager.FindByID(activateEquip.sID);
		if (equip && equip->CEquipType == EquipmentClass::Engine)
			reinterpret_cast<CShip*>(inspect->cobj)->set_throttle(0.0f);

		returncode = DEFAULT_RETURNCODE;
	}

	/* Activating cruise does not enable the engine if it was previously engine-killed. This breaks power draw computations. */
	void __stdcall ActivateCruise(unsigned int clientId, const XActivateCruise& activateCruise)
	{
		if (!activateCruise.bActivate)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(activateCruise.iShip, inspect, system) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		const auto& equip = reinterpret_cast<CEqObj*>(inspect->cobj)->equip_manager.FindFirst(EquipmentClass::Engine);
		if (equip)
		{
			XActivateEquip data;
			data.iSpaceID = activateCruise.iShip;
			data.bActivate = true;
			data.sID = equip->iSubObjId;
			Server.ActivateEquip(clientId, data);
		}

		returncode = DEFAULT_RETURNCODE;
	}

	/* Server does not sync with players' throttle. This is important for power draw and anti cheat. */
	void __stdcall SPObjUpdate(const SSPObjUpdateInfo& updateInfo, unsigned int clientId)
	{
		IObjRW* inspect;
		StarSystem* system;
		if (GetShipInspect(updateInfo.iShip, inspect, system) && inspect->cobj->objectClass & CObject::CSHIP_OBJECT)
		{
			if (updateInfo.throttle == 0.0f)
			{
				bool active = false;
				const auto& equip = reinterpret_cast<CEqObj*>(inspect->cobj)->equip_manager.FindFirst(EquipmentClass::Engine);
				if (equip)
					active = equip->IsActive();
				reinterpret_cast<CShip*>(inspect->cobj)->set_throttle(updateInfo.throttle);
				if (!active && equip)
					/* Restore the engine kill state after throttle was set. */
					equip->Activate(active);
			}
			else
				reinterpret_cast<CShip*>(inspect->cobj)->set_throttle(updateInfo.throttle);
		}

		returncode = DEFAULT_RETURNCODE;
	}

	/* Upon spawning a ship that is already in cruise, it will not send the cruise state. */
	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet)
	{
		IObjRW* inspect;
		StarSystem* system;
		if (GetShipInspect(packet.iSpaceID, inspect, system) && inspect->cobj->objectClass & CObject::CSHIP_OBJECT && reinterpret_cast<CShip*>(inspect->cobj)->is_cruise_active())
		{
			XActivateCruise activate;
			activate.iShip = packet.iSpaceID;
			activate.bActivate = true;
			GetClientInterface()->Send_FLPACKET_COMMON_ACTIVATECRUISE(clientId, activate);
		}

		returncode = DEFAULT_RETURNCODE;
		return true;
	}
}