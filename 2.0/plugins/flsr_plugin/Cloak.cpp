#include "Main.h"

namespace Cloak
{
	int cloakTimeInfoInterval = 5000;
	float jumpGateDecloakRadius = 2000.0f;
	float jumpHoleDecloakRadius = 1000.0f;
	std::string defaultCloakHardpoint = "HpCloak01";

	struct CloakingDeviceDefinition
	{
		uint activatorArchetypeId;
		uint cloakArchetypeId;
		float capacity;
		float powerUsage;
		float powerRecharge;
		float minRequiredCapacityToCloak;
		int cloakEffectDuration;
		int uncloakEffectDuration;
	};

	enum CloakState
	{
		Uncloaked,
		Cloaking,
		Cloaked,
		Uncloaking
	};

	struct ClientCloakingDevice
	{
		int activatorCargoId = 0;
		std::string activatorHardpoint = "";
		int cloakCargoId = 0;
		CloakState cloakState = CloakState::Cloaked;
		bool initialUncloakRequired = true;
		mstime cloakTimeStamp = 0;
		mstime uncloakTimeStamp = 0;
		mstime lastTimingInfoTimeStamp = 0;
		float capacity = 0.0f;
		bool insideNoCloakZone = false;
		bool dockingManeuverActive = false;
		CloakingDeviceDefinition cloakingDeviceDefinition;
	};

	enum CloakReturnState
	{
		None,
		Successful,
		Blocked,
		NoEnergy,
		NotReady
	};

	std::map<uint, ClientCloakingDevice> clientCloakingDevice;
	std::vector<CloakingDeviceDefinition> cloakingDeviceDefinitions;

	std::map<uint, std::vector<uint>> jumpGatesPerSystem;
	std::map<uint, std::vector<uint>> jumpHolesPerSystem;

	std::set<uint> clientIdsRequestingUncloak;
	std::map<std::wstring, float> lastPersistedCharacterCloakCapacity;

	bool IsValidCloakableClient(uint clientId)
	{
		if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId) && clientCloakingDevice.contains(clientId))
		{
			uint ship = 0;
			pub::Player::GetShip(clientId, ship);
			return ship;
		}
		return false;
	}

	bool IsFullyUncloaked(uint clientId)
	{
		return IsValidCloakableClient(clientId) && clientCloakingDevice[clientId].cloakState == CloakState::Uncloaked;
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

	void ClearClientData(uint clientId, bool storeCloakCapacity)
	{
		if (storeCloakCapacity && clientCloakingDevice.contains(clientId))
		{
			std::wstring characterFileNameWS;
			if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) == HKE_OK)
				lastPersistedCharacterCloakCapacity.insert({ characterFileNameWS, clientCloakingDevice[clientId].capacity });
		}

		clientCloakingDevice.erase(clientId);
		clientIdsRequestingUncloak.erase(clientId);
	}

	void LoadCloakSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + Globals::CLOAK_CONFIG_FILE;

		cloakingDeviceDefinitions.clear();

		const std::string generalSection = "General";
		defaultCloakHardpoint = IniGetS(configFilePath, generalSection, "CloakHardpoint", "HpCloak01");
		cloakTimeInfoInterval = IniGetI(configFilePath, generalSection, "CloakTimeInfoInterval", 5000);
		jumpGateDecloakRadius = IniGetF(configFilePath, generalSection, "JumpGateDecloakRadius", 2000.0f);
		jumpHoleDecloakRadius = IniGetF(configFilePath, generalSection, "JumpHoleDecloakRadius", 1000.0f);

		for (int i = 0;; i++)
		{
			const std::string section = "CloakDevice" + std::to_string(i);
			const std::string activatorNickname = IniGetS(configFilePath, section, "ActivatorNickname", "");
			const std::string cloakNickname = IniGetS(configFilePath, section, "CloakNickname", "");
			if (activatorNickname == "" || cloakNickname == "")
				break;
			CloakingDeviceDefinition definition;
			definition.activatorArchetypeId = CreateID(activatorNickname.c_str());
			definition.cloakArchetypeId = CreateID(cloakNickname.c_str());
			definition.capacity = IniGetF(configFilePath, section, "Capacity", 0.0f);
			definition.powerUsage = IniGetF(configFilePath, section, "PowerUsageWhileCloaked", 0.0f) / 1000.0f;
			definition.powerRecharge = IniGetF(configFilePath, section, "PowerRecharge", 0.0f) / 1000.0f;
			definition.minRequiredCapacityToCloak = 0.0f;
			definition.cloakEffectDuration = 0.0f;
			definition.uncloakEffectDuration = 0.0f;

			cloakingDeviceDefinitions.push_back(definition);
		}

		CollectAllJumpSolarsPerSystem();
	}

	bool initialized = false;

	// This must be executed AFTER LoadCloakSettings and when the game data has already been stored to memory.
	void InitializeWithGameData()
	{
		if (initialized)
			return;

		initialized = true;
		for (auto& definition : cloakingDeviceDefinitions)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(definition.cloakArchetypeId);
			if (equipment == 0)
				continue;
			const Archetype::CloakingDevice* archetype = (Archetype::CloakingDevice*)equipment;
			definition.minRequiredCapacityToCloak = definition.powerUsage * 1000 * archetype->fCloakinTime;
			definition.cloakEffectDuration = archetype->fCloakinTime * 1000;
			definition.uncloakEffectDuration = archetype->fCloakoutTime * 1000;
		}

		CollectAllJumpSolarsPerSystem();
	}

	bool EnsureCloakBeingEquipped(uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		std::list<CARGO_INFO> cargoList;
		int remainingCargoHoldSize;
		if (HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize) != HKE_OK)
			return false;

		for (const auto& cloakingDeviceDefinition : cloakingDeviceDefinitions)
		{
			for (const auto& cargo : cargoList)
			{
				if (cargo.bMounted && cargo.iArchID == cloakingDeviceDefinition.activatorArchetypeId)
				{
					const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
					if (HkAddEquip(ARG_CLIENTID(clientId), cloakingDeviceDefinition.cloakArchetypeId, defaultCloakHardpoint) == HKE_OK)
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

	void RemoveCloakingEquipment(uint clientId)
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

	bool FindCloakingEquipments(uint clientId, uint& activatorCargoId, std::string& activatorHardpoint, uint& cloakCargoId, CloakingDeviceDefinition& cloakingDeviceDefinition)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		std::list<CARGO_INFO> cargoList;
		int remainingCargoHoldSize;
		if (HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize) != HKE_OK)
			return false;

		for (auto& definition : cloakingDeviceDefinitions)
		{
			activatorCargoId = -1;
			cloakCargoId = -1;
			for (const auto& cargo : cargoList)
			{
				if (cargo.bMounted)
				{
					if (cargo.iArchID == definition.activatorArchetypeId)
					{
						activatorCargoId = cargo.iID;
						activatorHardpoint = std::string(cargo.hardpoint.value);
					}
					if (cargo.iArchID == definition.cloakArchetypeId)
						cloakCargoId = cargo.iID;
					if (activatorCargoId != -1 && cloakCargoId != -1)
					{
						cloakingDeviceDefinition = definition;
						return true;
					}
				}
			}
		}

		return false;
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

	void SendServerCloakingDeviceActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakingDevice[clientId].cloakCargoId;
		Server.ActivateEquip(clientId, activateEquipment);
	}

	void SendClientCloakingDeviceActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakingDevice[clientId].cloakCargoId;
		HookClient->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	void SendServerActivatorActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakingDevice[clientId].activatorCargoId;
		Server.ActivateEquip(clientId, activateEquipment);
	}

	void SendClientActivatorActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakingDevice[clientId].activatorCargoId;
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
					if (!line.empty() && line.find(clientCloakingDevice[clientId].activatorHardpoint) == std::string::npos)
						lines.push_back(currentWeaponGroups.substr(0, delimiterPos));
					currentWeaponGroups.erase(0, delimiterPos + 1);
				}

				const auto cloakState = clientCloakingDevice[clientId].cloakState;
				if (cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked)
				{
					for (int groupIndex = 0; groupIndex < 6; groupIndex++)
						lines.push_back("wg = " + std::to_string(groupIndex) + ", " + clientCloakingDevice[clientId].activatorHardpoint);
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

		const auto cloakState = clientCloakingDevice[clientId].cloakState;
		if (!HasMountedEquipmentByCargoId(clientId, clientCloakingDevice[clientId].activatorCargoId))
		{
			result = CloakReturnState::NotReady;
		}
		else if (cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking)
		{
			result = CloakReturnState::Successful;
		}
		else if (clientCloakingDevice[clientId].insideNoCloakZone || clientCloakingDevice[clientId].dockingManeuverActive || ClientInfo[clientId].bTradelane)
		{
			result = CloakReturnState::Blocked;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}
		else if (clientCloakingDevice[clientId].capacity < clientCloakingDevice[clientId].cloakingDeviceDefinition.minRequiredCapacityToCloak)
		{
			result = CloakReturnState::NoEnergy;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}
		else if (timeInMS() - clientCloakingDevice[clientId].uncloakTimeStamp < clientCloakingDevice[clientId].cloakingDeviceDefinition.uncloakEffectDuration)
		{
			result = CloakReturnState::NotReady;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}

		if (result == CloakReturnState::None)
		{
			clientCloakingDevice[clientId].cloakTimeStamp = timeInMS();
			clientCloakingDevice[clientId].cloakState = CloakState::Cloaking;
			SendServerCloakingDeviceActivationState(clientId, true);
			SendClientCloakingDeviceActivationState(clientId, true);
			SendServerActivatorActivationState(clientId, true);
			SendClientActivatorActivationState(clientId, true);
			SynchronizeWeaponGroupsWithCloakState(clientId);
			result = CloakReturnState::Successful;
		}

		if (result != CloakReturnState::Successful)
			clientIdsRequestingUncloak.erase(clientId);

		return result;
	}

	bool TryUncloak(uint clientId)
	{
		const auto cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Uncloaked || cloakState == CloakState::Uncloaking)
		{
			clientIdsRequestingUncloak.erase(clientId);
			return true;
		}

		if (timeInMS() - clientCloakingDevice[clientId].cloakTimeStamp > clientCloakingDevice[clientId].cloakingDeviceDefinition.cloakEffectDuration)
		{
			if (!HasMountedEquipmentByCargoId(clientId, clientCloakingDevice[clientId].activatorCargoId))
				return false;

			clientIdsRequestingUncloak.erase(clientId);
			clientCloakingDevice[clientId].uncloakTimeStamp = timeInMS();
			clientCloakingDevice[clientId].cloakState = CloakState::Uncloaking;
			clientCloakingDevice[clientId].initialUncloakRequired = false;
			SendServerCloakingDeviceActivationState(clientId, false);
			SendClientCloakingDeviceActivationState(clientId, false);
			SendServerActivatorActivationState(clientId, false);
			SendClientActivatorActivationState(clientId, false);
			SynchronizeWeaponGroupsWithCloakState(clientId);
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("deactivated"));
			return true;
		}
		return false;
	}

	void QueueUncloak(uint clientId)
	{
		if (!IsValidCloakableClient(clientId) || clientCloakingDevice[clientId].initialUncloakRequired)
			return;

		if (!TryUncloak(clientId))
			clientIdsRequestingUncloak.insert(clientId);
	}

	void AttemptInitialUncloak(uint clientId)
	{
		if (IsValidCloakableClient(clientId) && clientCloakingDevice[clientId].initialUncloakRequired)
		{
			TryUncloak(clientId);
		}
	}

	void DropShield(uint clientId)
	{
		pub::SpaceObj::DrainShields(Players[clientId].iShipID);
	}

	void InstallCloak(uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		uint activatorCargoId;
		std::string activatorHardpoint;
		uint cloakCargoId;
		CloakingDeviceDefinition definition;
		if (FindCloakingEquipments(clientId, activatorCargoId, activatorHardpoint, cloakCargoId, definition))
		{
			clientCloakingDevice[clientId].cloakingDeviceDefinition = definition;
			clientCloakingDevice[clientId].activatorCargoId = activatorCargoId;
			clientCloakingDevice[clientId].activatorHardpoint = activatorHardpoint;
			clientCloakingDevice[clientId].cloakCargoId = cloakCargoId;

			std::wstring characterFileNameWS;
			if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) == HKE_OK && lastPersistedCharacterCloakCapacity.contains(characterFileNameWS))
			{
				lastPersistedCharacterCloakCapacity.erase(characterFileNameWS);
				clientCloakingDevice[clientId].capacity = lastPersistedCharacterCloakCapacity[characterFileNameWS];
			}
			else
			{
				clientCloakingDevice[clientId].capacity = clientCloakingDevice[clientId].cloakingDeviceDefinition.capacity;
			}
		}
	}

	bool CheckDockCall(uint ship, uint dockTargetId, uint dockPortIndex, enum DOCK_HOST_RESPONSE response)
	{
		const uint clientId = HkGetClientIDByShip(ship);
		if (!IsValidCloakableClient(clientId))
			return true;

		clientCloakingDevice[clientId].dockingManeuverActive = false;

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

		clientCloakingDevice[clientId].dockingManeuverActive = true;
		return true;
	}

	void UncloakInNoCloakZones(uint clientId, uint clientSystemId, uint clientShipId)
	{
		const auto cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Uncloaked || cloakState == CloakState::Uncloaking)
			return;
		
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

		clientCloakingDevice[clientId].insideNoCloakZone = insideNoCloakZone;
		if (insideNoCloakZone)
			QueueUncloak(clientId);
	}

	void UpdateStateByEffectTimings(uint clientId, mstime currentTime)
	{
		const auto& cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Cloaking)
		{
			clientCloakingDevice[clientId].cloakState = currentTime - clientCloakingDevice[clientId].cloakTimeStamp > clientCloakingDevice[clientId].cloakingDeviceDefinition.cloakEffectDuration ? CloakState::Cloaked : CloakState::Cloaking;
		}
		else if (cloakState == CloakState::Uncloaking)
		{
			clientCloakingDevice[clientId].cloakState = currentTime - clientCloakingDevice[clientId].uncloakTimeStamp > clientCloakingDevice[clientId].cloakingDeviceDefinition.uncloakEffectDuration ? CloakState::Uncloaked : CloakState::Uncloaking;
		}
	}

	void UpdateCapacity(uint clientId, mstime currentTime, mstime previousTime)
	{
		if (clientCloakingDevice[clientId].cloakingDeviceDefinition.capacity == 0)
			return;

		const auto cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked)
		{
			clientCloakingDevice[clientId].capacity -= clientCloakingDevice[clientId].cloakingDeviceDefinition.powerUsage * (currentTime - previousTime);
			if (clientCloakingDevice[clientId].capacity <= 0.0f)
			{
				clientCloakingDevice[clientId].capacity = 0.0f;
				QueueUncloak(clientId);
			}
		}
		else if (cloakState == CloakState::Uncloaked)
		{
			clientCloakingDevice[clientId].capacity += clientCloakingDevice[clientId].cloakingDeviceDefinition.powerRecharge * (currentTime - previousTime);
			clientCloakingDevice[clientId].capacity = std::min(clientCloakingDevice[clientId].capacity, clientCloakingDevice[clientId].cloakingDeviceDefinition.capacity);
		}
	}

	void PrintRemainingCloakTime(uint clientId)
	{
		const int seconds = clientCloakingDevice[clientId].cloakingDeviceDefinition.powerUsage > 0 ? clientCloakingDevice[clientId].capacity  / (clientCloakingDevice[clientId].cloakingDeviceDefinition.powerUsage * 1000) : 0;
		PrintUserCmdText(clientId, L"Cloak time remaining: " + std::to_wstring(seconds) + L"s");
		clientCloakingDevice[clientId].lastTimingInfoTimeStamp = timeInMS();
	}

	void PrintTimeUntilRecharged(uint clientId)
	{
		const int timeDifference = std::min(clientCloakingDevice[clientId].cloakingDeviceDefinition.uncloakEffectDuration * 1000 + static_cast<int>(clientCloakingDevice[clientId].uncloakTimeStamp - timeInMS()), 0);
		const int seconds = clientCloakingDevice[clientId].cloakingDeviceDefinition.powerRecharge > 0 ? (clientCloakingDevice[clientId].cloakingDeviceDefinition.capacity - clientCloakingDevice[clientId].capacity) / (clientCloakingDevice[clientId].cloakingDeviceDefinition.powerRecharge * 1000) + timeDifference : 0;
		PrintUserCmdText(clientId, L"Cloak until recharged completely: " + std::to_wstring(seconds) + L"s");
		clientCloakingDevice[clientId].lastTimingInfoTimeStamp = timeInMS();
	}

	void PrintCloakPowerInformation(uint clientId)
	{
		const auto cloakState = clientCloakingDevice[clientId].cloakState;
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

			if (!HasMountedEquipmentByCargoId(clientId, clientCloakingDevice[clientId].activatorCargoId))
			{
				ClearClientData(clientId, false);
				continue;
			}

			// Synchronize cloak state to all players
			const auto cloakState = clientCloakingDevice[clientId].cloakState;
			SendServerCloakingDeviceActivationState(clientId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Synchronize activator state to all players
			SendServerActivatorActivationState(clientId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);
			SendClientActivatorActivationState(clientId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Uncloak player in no-cloak-zones
			UncloakInNoCloakZones(clientId, playerData->iSystemID, playerData->iShipID);

			// The rest in the update loop should be ignored for not initially uncloaked ships.
			if (clientCloakingDevice[clientId].initialUncloakRequired)
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

			if (clientCloakingDevice[clientId].capacity < clientCloakingDevice[clientId].cloakingDeviceDefinition.capacity && (now - clientCloakingDevice[clientId].lastTimingInfoTimeStamp) >= cloakTimeInfoInterval)
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

		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId) && clientCloakingDevice[clientId].activatorCargoId == activateEquip.sID)
		{
			ToggleClientCloakActivator(clientId, activateEquip.bActivate);
		}
	}

	void __stdcall FireWeapon(unsigned int clientId, struct XFireWeaponInfo const& fireWeaponInfo)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId) && clientCloakingDevice[clientId].activatorCargoId == *fireWeaponInfo.sHpIdsBegin)
		{
			const auto cloakState = clientCloakingDevice[clientId].cloakState;
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
				clientCloakingDevice[clientId].dockingManeuverActive = false;
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
			ClearClientData(clientId, false);
			RemoveCloakingEquipment(clientId);
		}
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			EnsureCloakBeingEquipped(clientId);
		}
	}

	void __stdcall CharacterSelect(struct CHARACTER_ID const& characterId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			ClearClientData(clientId, true);
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
			ClearClientData(clientId, true);
		}
	}
}