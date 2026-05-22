#include "CounterMeasureRecharge.h"
#include "Plugin.h"

namespace CounterMeasuresRecharge
{
	struct CounterMeasureData
	{
		int ammoLimit = 0;
		mstime rechargeDelay = 1000;
	};
	std::unordered_map<uint, CounterMeasureData> counterMeasureDataByArchId;

	struct PlayerCounterMeasureData
	{
		std::unordered_set<CECounterMeasureDropper*> droppers;
		std::unordered_map<CECounterMeasureDropper*, mstime> nextRechargeByDropper;
		bool cruising = false;
		ushort engineObjId = 0;
	};
	std::unordered_map<uint, PlayerCounterMeasureData> playerDataByClientId;

	void ReadFiles()
	{
		std::string dataPath = "..\\data";;
		std::vector<std::string> equipmentPaths;
		INI_Reader ini;
		if (ini.open("freelancer.ini", false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Freelancer"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("data path"))
						{
							dataPath = ini.get_value_string(0);
							break;
						}
					}
				}

				if (ini.is_header("Data"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("equipment"))
							equipmentPaths.push_back(ini.get_value_string(0));
					}
				}
			}
			ini.close();
		}

		for (const auto& equipPath : equipmentPaths)
		{
			if (ini.open((dataPath + "\\" + equipPath).c_str(), false))
			{
				while (ini.read_header())
				{
					if (ini.is_header("CounterMeasure"))
					{
						uint id = 0;
						int ammoLimit = MAX_PLAYER_AMMO;
						mstime ammoRefillDelay = 1000;
						bool ammoRequired = false;
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
								id = CreateID(ini.get_value_string(0));

							if (ini.is_value("ammo_limit"))
								ammoLimit = ini.get_value_int(0);

							if (ini.is_value("ammo_refill_delay"))
								ammoRefillDelay = static_cast<mstime>(ini.get_value_float(0) * 1000);

							if (ini.is_value("requires_ammo"))
								ammoRequired = ini.get_value_bool(0);
						}

						if (ammoRequired && id && ammoLimit > 0 && ammoRefillDelay >= 0)
							counterMeasureDataByArchId.insert({ id, { ammoLimit, ammoRefillDelay } });
					}
				}
				ini.close();
			}
		}
	}

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		ConPrint(L"Initializing Counter Measure Recharges... ");
		
		ReadFiles();
		
		ConPrint(L"Done\n");
	}

	void __stdcall FireWeapon(uint clientId, const XFireWeaponInfo& weapon)
	{
		if (auto dataEntry = playerDataByClientId.find(clientId); dataEntry != playerDataByClientId.end())
		{
			for (const auto& firedSubObjId : weapon.hpIds)
				for (const auto& dropper : dataEntry->second.droppers)
				{
					if (dropper->iSubObjId == firedSubObjId && dropper->CanFire(weapon.target) == FireResult::Success)
					{
						dataEntry->second.nextRechargeByDropper.at(dropper) = timeInMS() + counterMeasureDataByArchId.at(dropper->CounterMeasureArch()->iArchID).rechargeDelay;
						returncode = DEFAULT_RETURNCODE;
						return;
					}
				}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ActivateEquip(unsigned int clientId, const XActivateEquip& activateEquip)
	{
		// Only catch the case when engine gets deactivated.
		if (activateEquip.bActivate)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		if (auto dataEntry = playerDataByClientId.find(clientId); dataEntry != playerDataByClientId.end() && dataEntry->second.engineObjId == activateEquip.sID && dataEntry->second.cruising)
		{
			dataEntry->second.cruising = false;
			const mstime now = timeInMS();
			for (const auto& dropper : dataEntry->second.droppers)
				dataEntry->second.nextRechargeByDropper.at(dropper) = now + counterMeasureDataByArchId.at(dropper->CounterMeasureArch()->iArchID).rechargeDelay;
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ActivateCruise(unsigned int clientId, const XActivateCruise& activateCruise)
	{
		if (auto dataEntry = playerDataByClientId.find(clientId); dataEntry != playerDataByClientId.end())
		{
			dataEntry->second.cruising = activateCruise.bActivate;
			if (!activateCruise.bActivate)
			{
				const mstime now = timeInMS();
				for (const auto& dropper : dataEntry->second.droppers)
					dataEntry->second.nextRechargeByDropper.at(dropper) = now + counterMeasureDataByArchId.at(dropper->CounterMeasureArch()->iArchID).rechargeDelay;
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
	{
		/* Make sure there is no possible garbage data left from unhandled cases of player leaving space before. */
		playerDataByClientId.erase(clientId);
		IObjRW* inspect;
		StarSystem* system;
		if (GetShipInspect(shipId, inspect, system))
		{
			CEquipManager& equipManager = reinterpret_cast<CEqObj*>(inspect->cobj)->equip_manager;
			CEquipTraverser traverser(EquipmentClass::CM);
			CECounterMeasureDropper* equip = nullptr;
			while (equip = reinterpret_cast<CECounterMeasureDropper*>(equipManager.Traverse(traverser)))
			{
				const uint ammoArchId = equip->CounterMeasureArch()->iArchID;
				if (equip->IsConnected() && !equip->IsDestroyed() && counterMeasureDataByArchId.contains(ammoArchId))
				{
					playerDataByClientId[clientId].droppers.insert(equip);
					playerDataByClientId[clientId].nextRechargeByDropper.insert({ equip, counterMeasureDataByArchId.at(ammoArchId).rechargeDelay });
				}
			}

			if (auto dataEntry = playerDataByClientId.find(clientId); dataEntry != playerDataByClientId.end())
			{
				const auto& equip = equipManager.FindFirst(EquipmentClass::Engine);
				if (equip)
					dataEntry->second.engineObjId = equip->iSubObjId;
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipEquipDestroyed(const IObjRW* object, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* damageList)
	{
		const uint clientId = object->cobj->ownerPlayer;
		if (clientId)
		{
			if (auto dataEntry = playerDataByClientId.find(clientId); dataEntry != playerDataByClientId.end())
			{
				auto& droppers = dataEntry->second.droppers;
				if (const auto& dropperEntry = droppers.find((CECounterMeasureDropper*)equip); dropperEntry != droppers.end())
				{
					dataEntry->second.nextRechargeByDropper.erase(*dropperEntry);
					droppers.erase(dropperEntry);
					if (droppers.empty())
						playerDataByClientId.erase(dataEntry);
				}
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
	{
		/* If a player gets beamed back to base while being in loading screen after Undock, this code is skipped. No ship exists to be despawned. */
		const uint clientId = killedObject->cobj->ownerPlayer;
		if (clientId)
			playerDataByClientId.erase(clientId);

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId)
	{
		/* Make sure the data is always cleared, even if no ship was destroyed in special cases. */
		playerDataByClientId.erase(clientId);
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

		for (auto& dataEntry : playerDataByClientId)
		{
			if (dataEntry.second.cruising)
				continue;

			bool needsUpdate = false;
			for (const auto& rechargeEntry : dataEntry.second.nextRechargeByDropper)
				if (rechargeEntry.second <= now)
				{
					needsUpdate = true;
					break;
				}
			if (!needsUpdate)
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
				if (dataEntry.second.nextRechargeByDropper.at(dropper) > now)
					continue;

				const uint ammoArchId = dropper->CounterMeasureArch()->iArchID;
				bool ammoNeedsRefill = true;
				CEquipTraverser traverser(EquipmentClass::Cargo);
				CECargo* equip;
				while (equip = reinterpret_cast<CECargo*>(equipManager.Traverse(traverser)))
				{
					if (equip->EquipArch()->iArchID == ammoArchId)
					{
						if (equip->count >= counterMeasureDataByArchId.at(ammoArchId).ammoLimit)
							ammoNeedsRefill = false;
						break;
					}
				}
				if (ammoNeedsRefill)
				{
					HkAddCargo(ARG_CLIENTID(dataEntry.first), ammoArchId, 1, false);
					dataEntry.second.nextRechargeByDropper.at(dropper) = now + counterMeasureDataByArchId.at(ammoArchId).rechargeDelay;
				}
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}
}