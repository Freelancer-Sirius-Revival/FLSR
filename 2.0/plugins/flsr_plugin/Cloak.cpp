#include "Main.h"

namespace Cloak
{
	const int CLOAK_TIME_INFO_INTERVAL = 5000;
	const float JUMP_GATE_DECLOAK_RADIUS = 2000.0f;
	const float JUMP_HOLE_DECLOAK_RADIUS = 1000.0f;

	enum CloakReturnState
	{
		None,
		Successful,
		Blocked,
		NoEnergy,
		NotReady
	};

	std::map<uint, ClientCloakingDevice> clientCloakingDevice;
	std::vector<CloakDeviceInfo> cloakingDeviceInfoList;

	std::map<uint, std::vector<uint>> jumpGatesPerSystem;
	std::map<uint, std::vector<uint>> jumpHolesPerSystem;

	std::set<uint> clientIdsRequestingUncloak;

	void ClearClientData(uint clientId)
	{
		clientCloakingDevice.erase(clientId);
	}

	void CollectAllJumpSolarsPerSystem()
	{
		CSolar* obj = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
		while (obj = static_cast<CSolar*>(obj->FindNext()))
		{
			uint type;
			pub::SpaceObj::GetType(obj->iID, type);
			if (type == OBJ_JUMP_GATE)
			{
				jumpGatesPerSystem[obj->iSystem].push_back(obj->iID);
			}
			else if (type == OBJ_JUMP_HOLE)
			{
				jumpHolesPerSystem[obj->iSystem].push_back(obj->iID);
			}
		}
	}
	void LoadCloakSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + Globals::CLOAK_CONFIG_FILE;

		cloakingDeviceInfoList.clear();
		
		for (int i = 0;; i++)
		{
			const std::string section = "CloakDevice" + std::to_string(i);
			const std::string cloakingDeviceNickname = IniGetS(configFilePath, section, "Nickname", "");
			if (cloakingDeviceNickname == "")
				break;
			CloakDeviceInfo cloakingDevice;
			cloakingDevice.archetypeId = CreateID(cloakingDeviceNickname.c_str());
			cloakingDevice.capacity = IniGetF(configFilePath, section, "Capacity", 0.0f);
			cloakingDevice.powerUsage = IniGetF(configFilePath, section, "PowerUsageWhileCloaked", 0.0f) / 1000.0f;
			cloakingDevice.powerRecharge = IniGetF(configFilePath, section, "PowerRecharge", 0.0f) / 1000.0f;
			cloakingDevice.minRequiredCapacityToCloak = 0.0f;
			cloakingDevice.cloakEffectDuration = 0.0f;
			cloakingDevice.uncloakEffectDuration = 0.0f;
			
			cloakingDeviceInfoList.push_back(cloakingDevice);
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
		for (auto& cloakingDevice : cloakingDeviceInfoList)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(cloakingDevice.archetypeId);
			if (equipment == 0)
				continue;
			const Archetype::CloakingDevice* archetype = (Archetype::CloakingDevice*)equipment;
			cloakingDevice.minRequiredCapacityToCloak = cloakingDevice.powerUsage * 1000 * archetype->fCloakinTime;
			cloakingDevice.cloakEffectDuration = archetype->fCloakinTime * 1000;
			cloakingDevice.uncloakEffectDuration = archetype->fCloakoutTime * 1000;
		}

		CollectAllJumpSolarsPerSystem();
	}

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
		if (!IsValidCloakableClient(clientId))
			return true;

		return clientCloakingDevice[clientId].cloakState == CloakState::Uncloaked;
	}

	void TriggerCloakingDeviceActivationState(uint clientId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = ClientInfo[clientId].iShip;
		activateEquipment.sID = clientCloakingDevice[clientId].cargoId;
		Server.ActivateEquip(clientId, activateEquipment);
	}

	CloakReturnState TryCloak(uint clientId)
	{
		CloakReturnState result = CloakReturnState::None;

		const auto cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking)
		{
			result = CloakReturnState::Successful;
		}
		else if (clientCloakingDevice[clientId].insideNoCloakZone || ClientInfo[clientId].bTradelane)
		{
			result = CloakReturnState::Blocked;
		}
		else if (clientCloakingDevice[clientId].capacity < clientCloakingDevice[clientId].cloakData.minRequiredCapacityToCloak)
		{
			result = CloakReturnState::NoEnergy;
		}
		else if (timeInMS() - clientCloakingDevice[clientId].uncloakTimeStamp < clientCloakingDevice[clientId].cloakData.uncloakEffectDuration)
		{
			result = CloakReturnState::NotReady;
		}

		if (result == CloakReturnState::None)
		{
			clientCloakingDevice[clientId].cloakTimeStamp = timeInMS();
			clientCloakingDevice[clientId].cloakState = CloakState::Cloaking;
			TriggerCloakingDeviceActivationState(clientId, true);
			PrintUserCmdText(clientId, L" Cloaking");
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

		if (timeInMS() - clientCloakingDevice[clientId].cloakTimeStamp > clientCloakingDevice[clientId].cloakData.cloakEffectDuration)
		{
			clientIdsRequestingUncloak.erase(clientId);
			clientCloakingDevice[clientId].uncloakTimeStamp = timeInMS();
			clientCloakingDevice[clientId].cloakState = CloakState::Uncloaking;
			clientCloakingDevice[clientId].initialUncloakRequired = false;
			TriggerCloakingDeviceActivationState(clientId, false);
			PrintUserCmdText(clientId, L" Uncloaking");
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

		std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);

		std::list<CARGO_INFO> cargoList;
		int remainingCargoHoldSize;
		if (HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize) != HKE_OK)
			return;
		
		for (const auto& cargo : cargoList)
		{
			if (cargo.bMounted)
			{
				for (const auto& cloakDevice : cloakingDeviceInfoList)
				{
					if (cargo.iArchID == cloakDevice.archetypeId)
					{
						clientCloakingDevice[clientId].cloakData = cloakDevice;
						clientCloakingDevice[clientId].cargoId = cargo.iID;
						clientCloakingDevice[clientId].capacity = clientCloakingDevice[clientId].cloakData.capacity;
						return;
					}
				}
			}
		}
	}

	bool CheckDockCall(uint ship, uint dockTargetId, uint dockPortIndex, enum DOCK_HOST_RESPONSE response)
	{
		const uint clientId = HkGetClientIDByShip(ship);
		if (!IsValidCloakableClient(clientId))
			return true;

		// dockPortIndex == -1 -> aborting the dock
		if (!dockTargetId || dockPortIndex == -1 || response == DOCK_HOST_RESPONSE::ACCESS_DENIED || response == DOCK_HOST_RESPONSE::DOCK_DENIED)
			return true;
			
		if (!IsFullyUncloaked(clientId))
		{
			PrintUserCmdText(clientId, L"Docking with activated cloak not possible!");
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cannot_dock"));
			return false;
		}

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
				if (HkDistance3DByShip(jumpGateId, clientShipId) < JUMP_GATE_DECLOAK_RADIUS)
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
				if (HkDistance3DByShip(jumpHoleId, clientShipId) < JUMP_HOLE_DECLOAK_RADIUS)
				{
					insideNoCloakZone = true;
					break;
				}
			}
		}

		clientCloakingDevice[clientId].insideNoCloakZone = insideNoCloakZone;
		if (insideNoCloakZone)
		{
			QueueUncloak(clientId);
		}
	}

	void UpdateStateByEffectTimings(uint clientId, mstime currentTime)
	{
		const auto& cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Cloaking)
		{
			clientCloakingDevice[clientId].cloakState = currentTime - clientCloakingDevice[clientId].cloakTimeStamp > clientCloakingDevice[clientId].cloakData.cloakEffectDuration ? CloakState::Cloaked : CloakState::Cloaking;
		}
		else if (cloakState == CloakState::Uncloaking)
		{
			clientCloakingDevice[clientId].cloakState = currentTime - clientCloakingDevice[clientId].uncloakTimeStamp > clientCloakingDevice[clientId].cloakData.uncloakEffectDuration ? CloakState::Uncloaked : CloakState::Uncloaking;
		}
	}

	void UpdateCapacity(uint clientId, mstime currentTime, mstime previousTime)
	{
		if (clientCloakingDevice[clientId].cloakData.capacity == 0)
			return;

		const auto cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked)
		{
			clientCloakingDevice[clientId].capacity -= clientCloakingDevice[clientId].cloakData.powerUsage * (currentTime - previousTime);
			if (clientCloakingDevice[clientId].capacity <= 0.0f)
			{
				clientCloakingDevice[clientId].capacity = 0.0f;
				QueueUncloak(clientId);
			}
		}
		else if (cloakState == CloakState::Uncloaked)
		{
			clientCloakingDevice[clientId].capacity += clientCloakingDevice[clientId].cloakData.powerRecharge * (currentTime - previousTime);
			clientCloakingDevice[clientId].capacity = std::min(clientCloakingDevice[clientId].capacity, clientCloakingDevice[clientId].cloakData.capacity);
		}
	}

	void PrintRemainingCloakTime(uint clientId)
	{
		const int seconds = clientCloakingDevice[clientId].cloakData.powerUsage > 0 ? clientCloakingDevice[clientId].capacity  / (clientCloakingDevice[clientId].cloakData.powerUsage * 1000) : 0;
		PrintUserCmdText(clientId, L"Cloak time remaining: " + std::to_wstring(seconds) + L"s");
		clientCloakingDevice[clientId].lastTimingInfoTimeStamp = timeInMS();
	}

	void PrintTimeUntilRecharged(uint clientId)
	{
		const int timeDifference = std::min(clientCloakingDevice[clientId].cloakData.uncloakEffectDuration * 1000 + static_cast<int>(clientCloakingDevice[clientId].uncloakTimeStamp - timeInMS()), 0);
		const int seconds = clientCloakingDevice[clientId].cloakData.powerRecharge > 0 ? (clientCloakingDevice[clientId].cloakData.capacity - clientCloakingDevice[clientId].capacity) / (clientCloakingDevice[clientId].cloakData.powerRecharge * 1000) + timeDifference : 0;
		PrintUserCmdText(clientId, L"Cloak completely recharged in: " + std::to_wstring(seconds) + L"s");
		clientCloakingDevice[clientId].lastTimingInfoTimeStamp = timeInMS();
	}

	void PrintCloakPowerInformation(uint clientId)
	{
		const auto cloakState = clientCloakingDevice[clientId].cloakState;
		if (cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked)
		{
			PrintRemainingCloakTime(clientId);
		}
		else
		{
			PrintTimeUntilRecharged(clientId);
		}
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
			if (!IsValidCloakableClient(clientId) || clientCloakingDevice[clientId].initialUncloakRequired)
				continue;

			// Synchronize cloak state to all players
			const auto cloakState = clientCloakingDevice[clientId].cloakState;
			TriggerCloakingDeviceActivationState(clientId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Cloak state update when effect time has passed
			UpdateStateByEffectTimings(clientId, now);

			// Uncloak player in no-cloak-zones
			UncloakInNoCloakZones(clientId, playerData->iSystemID, playerData->iShipID);

			// Power Capacity
			UpdateCapacity(clientId, now, lastSynchronizeTimeStamp);

			// Schedule cloak changes
			if (clientIdsRequestingUncloak.contains(clientId))
				QueueUncloak(clientId);

			if (!IsFullyUncloaked(clientId))
				DropShield(clientId);

			if (clientCloakingDevice[clientId].capacity < clientCloakingDevice[clientId].cloakData.capacity && (now - clientCloakingDevice[clientId].lastTimingInfoTimeStamp) >= CLOAK_TIME_INFO_INTERVAL)
			{
				PrintCloakPowerInformation(clientId);
			}

		}
		lastSynchronizeTimeStamp = now;
	}

	void UserCmd_CLOAK(uint clientId, const std::wstring& wscParam)
	{
		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
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
					PrintRemainingCloakTime(clientId);
			}
		}
	}

	void UserCmd_UNCLOAK(uint clientId, const std::wstring& wscParam)
	{
		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
		{
			TryUncloak(clientId);
			PrintTimeUntilRecharged(clientId);
		}
	}

	void UserCmd_CLOAK_TIME(uint clientId, const std::wstring& wscParam)
	{
		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
		{
			PrintRemainingCloakTime(clientId);
			PrintTimeUntilRecharged(clientId);
		}
	}
}