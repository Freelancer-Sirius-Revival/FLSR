#include "Main.h"

namespace Cloak
{
	int cloakTimeInfoInterval = 5000;
	float jumpGateDecloakRadius = 2000.0f;
	float jumpHoleDecloakRadius = 1000.0f;

	struct CloakStatsDefinition
	{
		uint activatorArchetypeId = -1;
		float capacity = 0.0f;
		float powerUsage = 0.0f;
		float powerRecharge = 0.0f;
	};

	struct ShipEffectDefinition
	{
		uint shipArchetypeId = -1;
		uint cloakingDeviceArchetypeId = -1;
		uint fuseId = -1;
		std::string hardpoint = "";
		int effectDuration = 0;
	};

	enum CloakState
	{
		Uncloaked,
		Cloaking,
		Cloaked,
		Uncloaking
	};

	struct ClientCloakStats
	{
		uint activatorCargoId = -1;
		std::string activatorHardpoint = "";
		uint cloakCargoId = -1;
		CloakState cloakState = CloakState::Cloaked;
		bool initialUncloakRequired = true;
		mstime cloakTimeStamp = 0;
		mstime uncloakTimeStamp = 0;
		mstime lastTimingInfoTimeStamp = 0;
		float capacity = 0.0f;
		float minRequiredCapacityToCloak = 0.0f;
		bool insideNoCloakZone = false;
		bool dockingManeuverActive = false;
		CloakStatsDefinition* statsDefinition = 0;
		ShipEffectDefinition* effectsDefinition = 0;
	};

	enum CloakReturnState
	{
		None,
		Successful,
		Blocked,
		NoEnergy,
		NotReady,
		NotInitialized
	};

	std::vector<CloakStatsDefinition> cloakDefinitions;
	std::vector<ShipEffectDefinition> shipEffects;
	std::map<uint, ClientCloakStats> clientCloakStats;

	std::map<uint, std::vector<uint>> jumpGatesPerSystem;
	std::map<uint, std::vector<uint>> jumpHolesPerSystem;

	std::set<uint> clientIdsRequestingUncloak;
	std::map<std::wstring, float> lastPersistedCharacterCloakCapacity;

	bool IsValidCloakableClient(uint clientId)
	{
		if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId) && clientCloakStats.contains(clientId))
		{
			uint ship = 0;
			pub::Player::GetShip(clientId, ship);
			return ship;
		}
		return false;
	}

	bool IsFullyUncloaked(uint clientId)
	{
		return IsValidCloakableClient(clientId) && clientCloakStats[clientId].cloakState == CloakState::Uncloaked;
	}

	void CollectAllJumpSolarsPerSystem()
	{
		CSolar* obj = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
		while (obj = static_cast<CSolar*>(obj->FindNext()))
		{
			uint type;
			pub::SpaceObj::GetType(obj->iID, type);
			if (type == OBJ_JUMP_GATE && obj->GetVisitValue() != 128)
			{
				jumpGatesPerSystem[obj->iSystem].push_back(obj->iID);
			}
			else if (type == OBJ_JUMP_HOLE && obj->GetVisitValue() != 128)
			{
				jumpHolesPerSystem[obj->iSystem].push_back(obj->iID);
			}
		}
	}

	void ClearClientData(uint clientId)
	{
		std::wstring characterFileNameWS;
		if (clientCloakStats.contains(clientId) && HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) == HKE_OK)
			lastPersistedCharacterCloakCapacity.insert({ characterFileNameWS, clientCloakStats[clientId].capacity });

		clientCloakStats.erase(clientId);
		clientIdsRequestingUncloak.erase(clientId);
	}

	void LoadCloakSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + Globals::CLOAK_CONFIG_FILE;

		shipEffects.clear();
		cloakDefinitions.clear();
		INI_Reader ini;
		if (ini.open(configFilePath.c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("General"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("cloak_time_info_interval"))
							cloakTimeInfoInterval = ini.get_value_int(0);
						else if (ini.is_value("jump_gate_decloak_radius"))
							jumpGateDecloakRadius = ini.get_value_float(0);
						else if (ini.is_value("jump_hole_decloak_radius"))
							jumpHoleDecloakRadius = ini.get_value_float(0);
					}
				}

				if (ini.is_header("Ship"))
				{
					ShipEffectDefinition definition;
					while (ini.read_value())
					{
						if (ini.is_value("ship_nickname"))
							definition.shipArchetypeId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("cloaking_device_nickname"))
							definition.cloakingDeviceArchetypeId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("fuse_name"))
							definition.fuseId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("hardpoint"))
							definition.hardpoint = ini.get_value_string(0);
					}
					if (definition.shipArchetypeId && definition.cloakingDeviceArchetypeId && definition.fuseId && !definition.hardpoint.empty())
						shipEffects.push_back(definition);
				}

				if (ini.is_header("Cloak"))
				{
					CloakStatsDefinition definition;
					while (ini.read_value())
					{
						if (ini.is_value("activator_nickname"))
							definition.activatorArchetypeId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("capacity"))
							definition.capacity = ini.get_value_float(0);
						else if (ini.is_value("power_usage_while_cloaked"))
							definition.powerUsage = ini.get_value_float(0) / 1000.0f;
						else if (ini.is_value("power_recharge"))
							definition.powerRecharge = ini.get_value_float(0) / 1000.0f;
					}
					if (definition.activatorArchetypeId)
						cloakDefinitions.push_back(definition);
				}
			}
			ini.close();
		}
	}

	bool initialized = false;

	// This must be executed AFTER LoadCloakSettings and when the game data has already been stored to memory.
	void InitializeWithGameData()
	{
		if (initialized)
			return;

		initialized = true;
		for (auto& shipEffect : shipEffects)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(shipEffect.cloakingDeviceArchetypeId);
			if (equipment == 0)
				continue;
			const Archetype::CloakingDevice* archetype = (Archetype::CloakingDevice*)equipment;
			shipEffect.effectDuration = std::min(archetype->fCloakinTime, archetype->fCloakoutTime) * 1000;
		}

		CollectAllJumpSolarsPerSystem();
	}

	bool EquipCloakingDevice(uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		uint shipArchetypeId;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		if (!shipArchetypeId)
			return false;

		uint cloakingDeviceArchetypeId = -1;
		std::string hardpoint = "";
		for (const auto& shipEffect : shipEffects)
		{
			if (shipEffect.shipArchetypeId == shipArchetypeId)
			{
				cloakingDeviceArchetypeId = shipEffect.cloakingDeviceArchetypeId;
				hardpoint = shipEffect.hardpoint;
				break;
			}
		}

		if (!cloakingDeviceArchetypeId || hardpoint.empty())
			return false;

		std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		std::list<CARGO_INFO> cargoList;
		int remainingCargoHoldSize;
		if (HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize) != HKE_OK)
			return false;

		for (const auto& cloakDefinition : cloakDefinitions)
		{
			for (const auto& cargo : cargoList)
			{
				if (cargo.bMounted && cargo.iArchID == cloakDefinition.activatorArchetypeId)
				{
					if (HkAddEquip(ARG_CLIENTID(clientId), cloakingDeviceArchetypeId, hardpoint) == HKE_OK)
					{
						// Anti Cheat
						char* szClassPtr;
						memcpy(&szClassPtr, &Players, 4);
						szClassPtr += 0x418 * (clientId - 1);
						ulong lCRC;
						__asm
						{
							pushad
							mov ecx, [szClassPtr]
							call[CRCAntiCheat_FLSR]
							mov[lCRC], eax
							popad
						}
						memcpy(szClassPtr + 0x320, &lCRC, 4);
						return true;
					}
					return false;
				}
			}
		}

		return false;
	}

	void RemoveCloakingDevices(uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		std::list<CARGO_INFO> cargoList;
		int remainingCargoHoldSize;
		if (HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize) != HKE_OK)
			return;

		for (const auto& cargo : cargoList)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(cargo.iArchID);
			if (equipment != 0 && equipment->get_class_type() == Archetype::AClassType::CLOAKING_DEVICE)
			{
				if (HkRemoveCargo(characterNameWS, cargo.iID, 1) != HKE_OK)
					return;
			}
		}
	}

	bool HasMountedEquipmentByCargoId(uint clientId, uint cargoId)
	{
		if (!HkIsValidClientID(clientId))
			return false;

		std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		std::list<CARGO_INFO> cargoList;
		int remainingCargoHoldSize;
		if (HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize) != HKE_OK)
			return false;

		for (const auto& cargo : cargoList)
		{
			if (cargo.bMounted && cargo.iID == cargoId)
				return true;
		}
		return false;
	}

	void InstallCloak(uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		ClientCloakStats clientStats;

		uint shipArchetypeId;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		for (int index = 0; index < shipEffects.size(); index++)
		{
			if (shipEffects[index].shipArchetypeId == shipArchetypeId)
			{
				clientStats.effectsDefinition = &shipEffects[index];
				break;
			}
		}
		if (!clientStats.effectsDefinition)
			return;

		std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		std::list<CARGO_INFO> cargoList;
		int remainingCargoHoldSize;
		if (HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize) != HKE_OK)
			return;

		for (int index = 0; index < cloakDefinitions.size(); index++)
		{
			for (const auto& cargo : cargoList)
			{
				if (cargo.bMounted)
				{
					if (cargo.iArchID == cloakDefinitions[index].activatorArchetypeId)
					{
						clientStats.activatorCargoId = cargo.iID;
						clientStats.activatorHardpoint = std::string(cargo.hardpoint.value);
					}
					if (cargo.iArchID == clientStats.effectsDefinition->cloakingDeviceArchetypeId)
						clientStats.cloakCargoId = cargo.iID;
				}

				if (clientStats.activatorCargoId != -1 && clientStats.cloakCargoId != -1)
				{
					clientStats.statsDefinition = &cloakDefinitions[index];
					break;
				}
			}
			if (clientStats.statsDefinition)
				break;
		}
		if (!clientStats.statsDefinition)
			return;

		clientStats.minRequiredCapacityToCloak = clientStats.statsDefinition->powerUsage * clientStats.effectsDefinition->effectDuration;

		std::wstring characterFileNameWS;
		if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) == HKE_OK && lastPersistedCharacterCloakCapacity.contains(characterFileNameWS))
		{
			clientStats.capacity = lastPersistedCharacterCloakCapacity[characterFileNameWS];
			lastPersistedCharacterCloakCapacity.erase(characterFileNameWS);
		}
		else
		{
			clientStats.capacity = clientStats.statsDefinition->capacity;
		}

		clientCloakStats.insert({ clientId, clientStats });
	}

	void StartFuse(uint clientId, uint fuseId)
	{
		const IObjInspectImpl* obj = HkGetInspect(clientId);
		if (obj)
			HkLightFuse((IObjRW*)obj, fuseId, 0.0f, 0.0f, 0.0f);
	}

	void StopFuse(uint clientId, uint fuseId)
	{
		const IObjInspectImpl* obj = HkGetInspect(clientId);
		if (obj)
			HkUnLightFuse((IObjRW*)obj, fuseId);
	}

	void SendServerCloakingDeviceActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakStats[clientId].cloakCargoId;
		Server.ActivateEquip(clientId, activateEquipment);
	}

	void SendClientCloakingDeviceActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakStats[clientId].cloakCargoId;
		HookClient->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	void SendServerActivatorActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakStats[clientId].activatorCargoId;
		Server.ActivateEquip(clientId, activateEquipment);
	}

	void SendClientActivatorActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakStats[clientId].activatorCargoId;
		HookClient->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	void SynchronizeWeaponGroupsWithCloakState(uint clientId)
	{
		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			if (HkGetClientIdFromPD(playerData) == clientId)
			{
				std::string currentWeaponGroups(playerData->weaponGroup.value.c_str());
				std::vector<std::string> lines;

				size_t delimiterPos;
				while ((delimiterPos = currentWeaponGroups.find('\n')) != std::string::npos)
				{
					std::string line = currentWeaponGroups.substr(0, delimiterPos);
					if (!line.empty() && line.find(clientCloakStats[clientId].activatorHardpoint) == std::string::npos)
						lines.push_back(currentWeaponGroups.substr(0, delimiterPos));
					currentWeaponGroups.erase(0, delimiterPos + 1);
				}

				const auto cloakState = clientCloakStats[clientId].cloakState;
				if (cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked)
				{
					for (int groupIndex = 0; groupIndex < 6; groupIndex++)
						lines.push_back("wg = " + std::to_string(groupIndex) + ", " + clientCloakStats[clientId].activatorHardpoint);
				}

				std::string newWeaponGroups;
				for (const auto& line : lines)
					newWeaponGroups.append(line + "\n");

				Server.SetWeaponGroup(clientId, (unsigned char*)newWeaponGroups.c_str(), newWeaponGroups.size() + 1);
				HookClient->Send_FLPACKET_COMMON_SET_WEAPON_GROUP(clientId, (unsigned char*)newWeaponGroups.c_str(), newWeaponGroups.size() + 1);
				return;
			}
		}
	}

	CloakReturnState TryCloak(uint clientId)
	{
		CloakReturnState result = CloakReturnState::None;

		const auto cloakState = clientCloakStats[clientId].cloakState;
		if (clientCloakStats[clientId].initialUncloakRequired)
		{
			result = CloakReturnState::NotInitialized;
		}
		else if (!HasMountedEquipmentByCargoId(clientId, clientCloakStats[clientId].activatorCargoId))
		{
			result = CloakReturnState::NotReady;
		}
		else if (cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking)
		{
			result = CloakReturnState::Successful;
		}
		else if (clientCloakStats[clientId].insideNoCloakZone || clientCloakStats[clientId].dockingManeuverActive || ClientInfo[clientId].bTradelane)
		{
			result = CloakReturnState::Blocked;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}
		else if (clientCloakStats[clientId].capacity < clientCloakStats[clientId].minRequiredCapacityToCloak)
		{
			result = CloakReturnState::NoEnergy;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}
		else if (timeInMS() - clientCloakStats[clientId].uncloakTimeStamp < clientCloakStats[clientId].effectsDefinition->effectDuration)
		{
			result = CloakReturnState::NotReady;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}

		if (result == CloakReturnState::None)
		{
			SendServerCloakingDeviceActivationState(clientId, true);
			SendClientCloakingDeviceActivationState(clientId, true);
			SendServerActivatorActivationState(clientId, true);
			SendClientActivatorActivationState(clientId, true);
			SynchronizeWeaponGroupsWithCloakState(clientId);
			StartFuse(clientId, clientCloakStats[clientId].effectsDefinition->fuseId);
			clientCloakStats[clientId].cloakTimeStamp = timeInMS();
			clientCloakStats[clientId].cloakState = CloakState::Cloaking;
			result = CloakReturnState::Successful;
		}

		if (result != CloakReturnState::Successful)
			clientIdsRequestingUncloak.erase(clientId);

		return result;
	}

	bool TryUncloak(uint clientId)
	{
		const auto cloakState = clientCloakStats[clientId].cloakState;
		if (cloakState == CloakState::Uncloaked || cloakState == CloakState::Uncloaking)
		{
			clientIdsRequestingUncloak.erase(clientId);
			return true;
		}

		if (timeInMS() - clientCloakStats[clientId].cloakTimeStamp > clientCloakStats[clientId].effectsDefinition->effectDuration)
		{
			if (!HasMountedEquipmentByCargoId(clientId, clientCloakStats[clientId].activatorCargoId))
				return false;

			clientIdsRequestingUncloak.erase(clientId);
			SendServerCloakingDeviceActivationState(clientId, false);
			SendClientCloakingDeviceActivationState(clientId, false);
			SendServerActivatorActivationState(clientId, false);
			SendClientActivatorActivationState(clientId, false);
			SynchronizeWeaponGroupsWithCloakState(clientId);
			if (!clientCloakStats[clientId].initialUncloakRequired)
			{
				StartFuse(clientId, clientCloakStats[clientId].effectsDefinition->fuseId);
				pub::Player::SendNNMessage(clientId, pub::GetNicknameId("deactivated"));
			}
			clientCloakStats[clientId].uncloakTimeStamp = timeInMS();
			clientCloakStats[clientId].cloakState = CloakState::Uncloaking;
			clientCloakStats[clientId].initialUncloakRequired = false;
			return true;
		}
		return false;
	}

	void QueueUncloak(uint clientId)
	{
		if (!IsValidCloakableClient(clientId) || clientCloakStats[clientId].initialUncloakRequired)
			return;

		if (!TryUncloak(clientId))
			clientIdsRequestingUncloak.insert(clientId);
	}

	void AttemptInitialUncloak(uint clientId)
	{
		if (IsValidCloakableClient(clientId) && clientCloakStats[clientId].initialUncloakRequired)
		{
			TryUncloak(clientId);
		}
	}

	void DropShield(uint clientId)
	{
		pub::SpaceObj::DrainShields(Players[clientId].iShipID);
	}

	bool CheckDockCall(uint ship, uint dockTargetId, uint dockPortIndex, enum DOCK_HOST_RESPONSE response)
	{
		const uint clientId = HkGetClientIDByShip(ship);
		if (!IsValidCloakableClient(clientId))
			return true;

		clientCloakStats[clientId].dockingManeuverActive = false;

		// dockPortIndex == -1 -> aborting the dock
		if (!dockTargetId || dockPortIndex == -1 || response == DOCK_HOST_RESPONSE::ACCESS_DENIED || response == DOCK_HOST_RESPONSE::DOCK_DENIED)
			return true;

		if (!IsFullyUncloaked(clientId))
		{
			PrintUserCmdText(clientId, L"Docking with activated cloak not possible!");
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cannot_dock"));
			
			// Cancel entering Formation when their docking is in process.
			uint dockTargetType;
			pub::SpaceObj::GetType(dockTargetId, dockTargetType);
			if (dockTargetType == OBJ_JUMP_HOLE || dockTargetType == OBJ_JUMP_GATE)
			{
				Vector formationOffset;
				HookClient->Send_FLPACKET_SERVER_FORMATION_UPDATE(clientId, ship, formationOffset);
			}
			return false;
		}

		clientCloakStats[clientId].dockingManeuverActive = true;
		return true;
	}

	void CheckPlayerInNoCloakZones(uint clientId, uint clientSystemId, uint clientShipId)
	{
		bool insideNoCloakZone = false;
		if (jumpGatesPerSystem.contains(clientSystemId))
		{
			for (const uint& jumpGateId : jumpGatesPerSystem[clientSystemId])
			{
				if (HkDistance3DByShip(jumpGateId, clientShipId) < jumpGateDecloakRadius)
				{
					insideNoCloakZone = true;
					break;
				}
			}
		}
		if (!insideNoCloakZone && jumpHolesPerSystem.contains(clientSystemId))
		{
			for (const uint& jumpHoleId : jumpHolesPerSystem[clientSystemId])
			{
				if (HkDistance3DByShip(jumpHoleId, clientShipId) < jumpHoleDecloakRadius)
				{
					insideNoCloakZone = true;
					break;
				}
			}
		}

		clientCloakStats[clientId].insideNoCloakZone = insideNoCloakZone;
		const auto cloakState = clientCloakStats[clientId].cloakState;
		if (insideNoCloakZone && (cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking))
			QueueUncloak(clientId);
	}

	void UpdateStateByEffectTimings(uint clientId, mstime currentTime)
	{
		auto& cloakState = clientCloakStats[clientId].cloakState;
		if (cloakState == CloakState::Cloaking)
		{
			cloakState = currentTime - clientCloakStats[clientId].cloakTimeStamp > clientCloakStats[clientId].effectsDefinition->effectDuration ? CloakState::Cloaked : CloakState::Cloaking;
		}
		else if (cloakState == CloakState::Uncloaking)
		{
			cloakState = currentTime - clientCloakStats[clientId].uncloakTimeStamp > clientCloakStats[clientId].effectsDefinition->effectDuration ? CloakState::Uncloaked : CloakState::Uncloaking;
		}
		if (cloakState == CloakState::Cloaked || cloakState == CloakState::Uncloaked)
		{
			StopFuse(clientId, clientCloakStats[clientId].effectsDefinition->fuseId);
		}
	}

	void UpdateCapacity(uint clientId, mstime currentTime, mstime previousTime)
	{
		if (clientCloakStats[clientId].statsDefinition->capacity == 0)
			return;

		const auto cloakState = clientCloakStats[clientId].cloakState;
		if (cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked)
		{
			clientCloakStats[clientId].capacity -= clientCloakStats[clientId].statsDefinition->powerUsage * (currentTime - previousTime);
			if (clientCloakStats[clientId].capacity <= 0.0f)
			{
				clientCloakStats[clientId].capacity = 0.0f;
				QueueUncloak(clientId);
			}
		}
		else if (cloakState == CloakState::Uncloaked)
		{
			clientCloakStats[clientId].capacity += clientCloakStats[clientId].statsDefinition->powerRecharge * (currentTime - previousTime);
			clientCloakStats[clientId].capacity = std::min(clientCloakStats[clientId].capacity, clientCloakStats[clientId].statsDefinition->capacity);
		}
	}

	void PrintRemainingCloakTime(uint clientId)
	{
		const int seconds = clientCloakStats[clientId].statsDefinition->powerUsage > 0 ? clientCloakStats[clientId].capacity  / (clientCloakStats[clientId].statsDefinition->powerUsage * 1000) : 0;
		PrintUserCmdText(clientId, L"Cloak time remaining: " + std::to_wstring(seconds) + L"s");
		clientCloakStats[clientId].lastTimingInfoTimeStamp = timeInMS();
	}

	void PrintTimeUntilRecharged(uint clientId)
	{
		const int timeDifference = std::min(clientCloakStats[clientId].effectsDefinition->effectDuration * 1000 + static_cast<int>(clientCloakStats[clientId].uncloakTimeStamp - timeInMS()), 0);
		const int seconds = clientCloakStats[clientId].statsDefinition->powerRecharge > 0 ? (clientCloakStats[clientId].statsDefinition->capacity - clientCloakStats[clientId].capacity) / (clientCloakStats[clientId].statsDefinition->powerRecharge * 1000) + timeDifference : 0;
		PrintUserCmdText(clientId, L"Cloak until recharged completely: " + std::to_wstring(seconds) + L"s");
		clientCloakStats[clientId].lastTimingInfoTimeStamp = timeInMS();
	}

	void PrintCloakPowerInformation(uint clientId)
	{
		const auto cloakState = clientCloakStats[clientId].cloakState;
		if (cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked)
			PrintRemainingCloakTime(clientId);
		else
			PrintTimeUntilRecharged(clientId);
	}

	mstime lastSynchronizeTimeStamp = 0;

	// This is executed to make sure players that spawn into a system with other cloaking players have their visibility synced.
	void SynchronizeCloakedClients()
	{
		if (!Modules::GetModuleState("CloakModule"))
			return;

		if (lastSynchronizeTimeStamp == 0)
			lastSynchronizeTimeStamp = timeInMS();

		const auto now = timeInMS();

		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			const uint clientId = HkGetClientIdFromPD(playerData);
			if (!IsValidCloakableClient(clientId))
				continue;

			if (!HasMountedEquipmentByCargoId(clientId, clientCloakStats[clientId].activatorCargoId))
			{
				ClearClientData(clientId);
				continue;
			}

			// Synchronize cloak state to all players
			const auto cloakState = clientCloakStats[clientId].cloakState;
			SendServerCloakingDeviceActivationState(clientId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Synchronize activator state to all players
			SendServerActivatorActivationState(clientId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);
			SendClientActivatorActivationState(clientId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Uncloak player in no-cloak-zones
			CheckPlayerInNoCloakZones(clientId, playerData->iSystemID, playerData->iShipID);

			// The rest in the update loop should be ignored for not initially uncloaked ships.
			if (clientCloakStats[clientId].initialUncloakRequired)
				continue;

			// Cloak state update when effect time has passed
			UpdateStateByEffectTimings(clientId, now);

			// Power Capacity
			UpdateCapacity(clientId, now, lastSynchronizeTimeStamp);

			// Schedule cloak changes
			if (clientIdsRequestingUncloak.contains(clientId))
				QueueUncloak(clientId);

			if (!IsFullyUncloaked(clientId))
				DropShield(clientId);

			if (clientCloakStats[clientId].capacity < clientCloakStats[clientId].statsDefinition->capacity && (now - clientCloakStats[clientId].lastTimingInfoTimeStamp) >= cloakTimeInfoInterval)
				PrintCloakPowerInformation(clientId);
		}
		lastSynchronizeTimeStamp = now;
	}

	void UserCmd_CLOAK_TIME(uint clientId, const std::wstring& wscParam)
	{
		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
		{
			PrintRemainingCloakTime(clientId);
			PrintTimeUntilRecharged(clientId);
		}
	}

	bool ToggleClientCloakActivator(uint clientId, bool active)
	{
		bool successful = false;
		if (active)
		{
			switch (TryCloak(clientId))
			{
				case CloakReturnState::Blocked:
					PrintUserCmdText(clientId, L"Cloak blocked!");
					break;

				case CloakReturnState::NoEnergy:
					PrintUserCmdText(clientId, L"Not enough cloaking energy!");
					break;

				case CloakReturnState::NotReady:
					PrintUserCmdText(clientId, L"Cloak not ready!");
					break;

				case CloakReturnState::NotInitialized:
					// Do not print anything in this case.
					break;

				default:
					successful = true;
					PrintRemainingCloakTime(clientId);
			}
		}
		else
		{
			successful = TryUncloak(clientId);
			PrintTimeUntilRecharged(clientId);
		}
		return successful;
	}

	void __stdcall ActivateEquip(unsigned int clientId, struct XActivateEquip const& activateEquip)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId) && clientCloakStats[clientId].activatorCargoId == activateEquip.sID)
		{
			ToggleClientCloakActivator(clientId, activateEquip.bActivate);
		}
	}

	void __stdcall FireWeapon(unsigned int clientId, struct XFireWeaponInfo const& fireWeaponInfo)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId) && clientCloakStats[clientId].activatorCargoId == *fireWeaponInfo.sHpIdsBegin)
		{
			const auto cloakState = clientCloakStats[clientId].cloakState;
			const bool activate = !(cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);
			if (ToggleClientCloakActivator(clientId, activate))
			{
				SendServerActivatorActivationState(clientId, activate);
				SendClientActivatorActivationState(clientId, activate);
			}

			// Prevent creating a projectile in space by the Activator.
			returncode = NOFUNCTIONCALL;
		}
	}

	void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			uint clientId = HkGetClientIDByShip(shipId);
			if (!clientId)
				return;

			QueueUncloak(clientId);
			if (IsValidCloakableClient(clientId))
				clientCloakStats[clientId].dockingManeuverActive = false;
		}
	}

	void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			InstallCloak(clientId);
		}
	}
	
	int __cdecl Dock_Call(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && !CheckDockCall(ship, dockTargetId, dockPortIndex, response))
		{
			returncode = NOFUNCTIONCALL;
		}

		return 0;
	}

	void __stdcall GoTradelane(unsigned int clientId, struct XGoTradelane const& goToTradelane)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			QueueUncloak(clientId);
		}
	}

	void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			ClearClientData(clientId);
			RemoveCloakingDevices(clientId);
		}
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			std::wstring characterFileNameWS;
			if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) == HKE_OK)
				lastPersistedCharacterCloakCapacity.erase(characterFileNameWS);
			EquipCloakingDevice(clientId);
		}
	}

	void __stdcall CharacterSelect(struct CHARACTER_ID const& characterId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			ClearClientData(clientId);
		}
	}

	void __stdcall SPObjUpdate(struct SSPObjUpdateInfo const& updateInfo, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			AttemptInitialUncloak(clientId);
		}
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			ClearClientData(clientId);
		}
	}
}