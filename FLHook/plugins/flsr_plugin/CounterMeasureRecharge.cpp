#include "CounterMeasureRecharge.h"
#include "Plugin.h"

namespace CounterMeasuresRecharge
{
	const mstime RechargeDelayInMs = 5000;
	const uint TargetAmmoCount = 5;

	struct PlayerCounterMeasureData
	{
		std::unordered_set<CECounterMeasureDropper*> droppers;
		mstime nextRechargeTime = timeInMS() + RechargeDelayInMs;
		bool cruising = false;
	};

	std::unordered_map<uint, PlayerCounterMeasureData> dataByClientId;

	void __stdcall FireWeapon(uint clientId, const XFireWeaponInfo& weapon)
	{
		if (auto dataEntry = dataByClientId.find(clientId); dataEntry != dataByClientId.end())
		{
			for (const auto& firedSubObjId : weapon.hpIds)
				for (const auto& dropper : dataEntry->second.droppers)
				{
					if (dropper->iSubObjId == firedSubObjId && dropper->CanFire(weapon.target) == FireResult::Success)
					{
						dataEntry->second.nextRechargeTime = timeInMS() + RechargeDelayInMs;
						returncode = DEFAULT_RETURNCODE;
						return;
					}
				}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ActivateCruise(unsigned int clientId, const XActivateCruise& activateCruise)
	{
		if (auto dataEntry = dataByClientId.find(clientId); dataEntry != dataByClientId.end())
		{
			dataEntry->second.cruising = activateCruise.bActivate;
			if (!activateCruise.bActivate)
				dataEntry->second.nextRechargeTime = timeInMS() + RechargeDelayInMs;
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
	{
		IObjRW* inspect;
		StarSystem* system;
		if (GetShipInspect(shipId, inspect, system))
		{
			CEquipManager& equipManager = reinterpret_cast<CEqObj*>(inspect->cobj)->equip_manager;
			CEquipTraverser traverser(EquipmentClass::CM);
			CECounterMeasureDropper* equip;
			while (equip = reinterpret_cast<CECounterMeasureDropper*>(equipManager.Traverse(traverser)))
			{
				if (equip->IsConnected() && !equip->IsDestroyed())
					dataByClientId[clientId].droppers.insert(equip);
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipEquipDestroyed(const IObjRW* object, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* damageList)
	{
		const uint clientId = object->cobj->ownerPlayer;
		if (clientId)
		{
			if (auto dataEntry = dataByClientId.find(clientId); dataEntry != dataByClientId.end())
			{
				auto& droppers = dataEntry->second.droppers;
				if (const auto dropperEntry = droppers.find((CECounterMeasureDropper*)equip); dropperEntry != droppers.end())
				{
					droppers.erase(dropperEntry);
					if (droppers.empty())
						dataByClientId.erase(dataEntry);
				}
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
	{
		const uint clientId = killedObject->cobj->ownerPlayer;
		if (clientId)
			dataByClientId.erase(clientId);

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId)
	{
		std::unordered_set<ushort> ammoObjIds;
		for (const auto& equip : Players[clientId].equipDescList.equip)
		{
			const auto& archetype = Archetype::GetEquipment(equip.iArchID);
			if (archetype && archetype->get_class_type() == Archetype::COUNTER_MEASURE)
				ammoObjIds.insert(equip.sID);
		}
		for (const auto& ammoId : ammoObjIds)
			HkRemoveCargo(ARG_CLIENTID(clientId), ammoId, MAXINT32);

		returncode = DEFAULT_RETURNCODE;
	}

	float elapsedTime = 0.0f;
	void __stdcall Elapse_Time_After(float seconds)
	{
		elapsedTime += seconds;
		if (elapsedTime < 0.1f)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}
		elapsedTime = 0.0f;

		const mstime now = timeInMS();

		for (auto& dataEntry : dataByClientId)
		{
			if (dataEntry.second.cruising || dataEntry.second.nextRechargeTime > now)
				continue;
			
			uint shipId = 0;
			pub::Player::GetShip(dataEntry.first, shipId);
			IObjRW* inspect;
			StarSystem* system;
			if (!shipId || !GetShipInspect(shipId, inspect, system))
				continue;
			
			CEquipManager& equipManager = reinterpret_cast<CEqObj*>(inspect->cobj)->equip_manager;
			for (const auto& dropper : dataEntry.second.droppers)
			{
				const uint ammoArchId = dropper->CounterMeasureArch()->iArchID;
				bool ammoNeedsRefill = true;
				CEquipTraverser traverser(EquipmentClass::Cargo);
				CECargo* equip;
				while (equip = reinterpret_cast<CECargo*>(equipManager.Traverse(traverser)))
				{
					if (equip->EquipArch()->iArchID == ammoArchId)
					{
						if (equip->count >= TargetAmmoCount)
							ammoNeedsRefill = false;
						break;
					}
				}
				if (ammoNeedsRefill)
				{
					HkAddCargo(ARG_CLIENTID(dataEntry.first), ammoArchId, 1, false);
					dataEntry.second.nextRechargeTime = timeInMS() + RechargeDelayInMs;
				}
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}
}