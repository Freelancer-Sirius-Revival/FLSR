#include "Main.h"

/**
 * CLOAKING PLUGIN
 * 
 * by Skotty
 * Thanks to Schmackbolzen, Aingar, Laz
 * 
 * 
 * Config template:
 * 
 * [General]
 * jump_gate_decloak_radius = 
 * jump_hole_decloak_radius = 
 * 
 * [Ship]
 * ship_nickname = 
 * cloaking_device_nickname = 
 * cloaking_device_hardpoint = 
 * cloak_fuse_name = 
 * uncloak_fuse_name = 
 * 
 * [Cloak]
 * activator_nickname = 
 * power_nickname = 
 * 
 * 
 * General Functionality:
 * 
 * * `activator_name` is a proxy-equipment which acts as way to enable/disable the cloaking device. Ideally this is a launcher/gun to show in the weapons list.
 *    Activation can be done by either using the hot-keys for enable/disable weapon, or by firing it (works only to enable, as cloak blocks afterwards to disable it).
 * 
 * * `power_nickname` is a power plant which is being dynamically added to the ship after undock. This will serve as energy-drain. It should have negative charge_rate!
 * 
 * * `ship_nickname` is a specific ship the following parameters will be tailored to.
 * 
 * * `cloaking_device_nickname` is a cloaking device which is being dynamically added to the ship after undock. It serves only the purpose of hiding the player,
 *   and to enable the transition from visible to invisible. The cloak-times should both be the same and match the desired effect's length!
 *   The actual cloak-effects should be set empty/removed - a fuse will do the job.
 * 
 * * `cloak_fuse_name` is a fuse which is being created as effect when cloaking. This is done to avoid the game bug of the uncloak effect being always detached from the ship.
 * 
 * * `uncloak_fuse_name` is a fuse which is being created as effect when uncloaking. This is done to avoid the game bug of the uncloak effect being always detached from the ship.
 * 
 * * `hardpoint` is the name of the hardpont where the Cloaking Device will be mounted to. Ideally this should be inside the ship and parented to the main hull.
 *   The Cloaking Device should never be shot off (explosion_resistance = 0) to avoid being unable to uncloak when losing a wing.
 * 
 * * Shields will be kept disabled while being not fully uncloaked
 * 
 * 
 * Specific Behavior:
 * 
 * Cloaking devices will be automatically uncloaked when:
 * * Ship energy reaches zero
 * * flying into a No-Cloak-Zone (Jump-Gates/Jump-Holes)
 * * entering a Tradelane
 * 
 * Cloaking is being blocked when:
 * * being in a running docking sequence
 * * jumping through a jump-tunnel until reaching the other side fully
 * * being in a Tradelane
 * 
 * Maneuver will be blocken when:
 * * trying to dock anywhere while not being fully uncloaked
 * * trying to go in formation to docking ships while not being fully uncloaked
 * 
 * Formation will be automatically broken when:
 * * going into a docking sequence while being in formation and not fully uncloaked
 * 
 */

namespace Cloak
{
	const uint SHIELD_SLOT_ID = 65521;
	const std::string BAY_HARDPOINT = "BAY";
	const uint TIMER_INTERVAL = 200;

	float jumpGateDecloakRadius = 2000.0f;
	float jumpHoleDecloakRadius = 1000.0f;

	struct CloakStatsDefinition
	{
		uint activatorArchetypeId = 0;
		uint powerArchetypeId = 0;
		float powerUsage = 0.0f;
	};

	struct ShipEffectDefinition
	{
		uint shipArchetypeId = 0;
		uint cloakingDeviceArchetypeId = 0;
		uint cloakFuseId = 0;
		uint uncloakFuseId = 0;
		int cloakDuration = 0;
		int uncloakDuration = 0;
		std::string cloakingDeviceHardpoint = "";
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
		CShip* ship = 0;
		IObjInspectImpl* shipInspect = 0;
		uint activatorCargoId = 0;
		std::string activatorHardpoint = "";
		uint cloakCargoId = 0;
		uint powerId = 0;
		bool shieldPresent = false;
		CloakState cloakState = CloakState::Cloaked;
		bool initialUncloakCompleted = false;
		mstime cloakTimeStamp = 0;
		mstime uncloakTimeStamp = 0;
		bool insideNoCloakZone = false;
		bool dockingManeuverActive = false;
		CloakStatsDefinition* statsDefinition = 0;
		ShipEffectDefinition* effectsDefinition = 0;
	};

	enum CloakReturnState
	{
		None,
		Successful,
		DockSequence,
		Blocked,
		Destroyed,
		NotReady,
		NotInitialized
	};

	std::vector<CloakStatsDefinition> cloakDefinitions;
	std::vector<ShipEffectDefinition> shipEffects;
	std::map<uint, ClientCloakStats> clientCloakStats;

	std::map<uint, std::vector<uint>> jumpGatesPerSystem;
	std::map<uint, std::vector<uint>> jumpHolesPerSystem;

	std::set<uint> clientIdsRequestingUncloak;

	bool IsValidCloakableClient(uint clientId)
	{
		if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId) && clientCloakStats.contains(clientId))
		{
			uint shipId = 0;
			pub::Player::GetShip(clientId, shipId);
			return shipId;
		}
		return false;
	}

	bool IsFullyUncloaked(uint clientId)
	{
		return IsValidCloakableClient(clientId) && clientCloakStats[clientId].cloakState == CloakState::Uncloaked;
	}

	void CollectAllJumpSolarsPerSystem()
	{
		CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
		while (solar = static_cast<CSolar*>(solar->FindNext()))
		{
			uint type;
			pub::SpaceObj::GetType(solar->iID, type);
			if (type == OBJ_JUMP_GATE && jumpGateDecloakRadius > 0.0f && solar->GetParentNickname().IsEmpty())
			{
				jumpGatesPerSystem[solar->iSystem].push_back(solar->iID);
			}
			else if (type == OBJ_JUMP_HOLE && jumpHoleDecloakRadius > 0.0f && solar->GetParentNickname().IsEmpty())
			{
				jumpHolesPerSystem[solar->iSystem].push_back(solar->iID);
			}
		}
	}

	void ClearClientData(uint clientId)
	{
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
						if (ini.is_value("jump_gate_decloak_radius"))
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
						else if (ini.is_value("cloaking_device_hardpoint"))
							definition.cloakingDeviceHardpoint = ini.get_value_string(0);
						else if (ini.is_value("cloak_fuse_name"))
							definition.cloakFuseId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("uncloak_fuse_name"))
							definition.uncloakFuseId = CreateID(ini.get_value_string(0));
					}
					if (definition.shipArchetypeId && definition.cloakingDeviceArchetypeId && definition.cloakFuseId && definition.uncloakFuseId && !definition.cloakingDeviceHardpoint.empty())
						shipEffects.push_back(definition);
				}

				if (ini.is_header("Cloak"))
				{
					CloakStatsDefinition definition;
					while (ini.read_value())
					{
						if (ini.is_value("activator_nickname"))
							definition.activatorArchetypeId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("power_nickname"))
							definition.powerArchetypeId = CreateID(ini.get_value_string(0));
					}
					if (definition.activatorArchetypeId && definition.powerArchetypeId)
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
			if (!equipment)
				continue;
			const Archetype::CloakingDevice* archetype = (Archetype::CloakingDevice*)equipment;
			shipEffect.cloakDuration = archetype->fCloakinTime * 1000;
			shipEffect.uncloakDuration = archetype->fCloakoutTime * 1000;
		}

		for (auto& cloak : cloakDefinitions)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(cloak.powerArchetypeId);
			if (!equipment)
				continue;
			const Archetype::Power* archetype = (Archetype::Power*)equipment;
			cloak.powerUsage = archetype->fChargeRate / -1000 * TIMER_INTERVAL;
		}

		CollectAllJumpSolarsPerSystem();
	}

	bool EquipEquipment(uint clientId, uint archetypeId, std::string hardpoint)
	{
		if (HkAddEquip(ARG_CLIENTID(clientId), archetypeId, hardpoint) == HKE_OK)
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

	bool EquipCloakingDeviceAndPower(uint clientId)
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
				hardpoint = shipEffect.cloakingDeviceHardpoint;
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
					return EquipEquipment(clientId, cloakingDeviceArchetypeId, hardpoint) &&
						   EquipEquipment(clientId, cloakDefinition.powerArchetypeId, BAY_HARDPOINT);
				}
			}
		}

		return false;
	}

	void RemoveCloakingDevicesAndPower(uint clientId)
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
			if (!equipment)
				continue;

			const  auto archetypeType = equipment->get_class_type();
			if (archetypeType == Archetype::AClassType::CLOAKING_DEVICE)
			{
				HkRemoveCargo(characterNameWS, cargo.iID, 1);
			}
			else if (archetypeType == Archetype::AClassType::POWER)
			{
				for (const auto& cloakDefinition : cloakDefinitions)
				{
					if (cargo.iArchID == cloakDefinition.powerArchetypeId)
					{
						HkRemoveCargo(characterNameWS, cargo.iID, 1);
						break;
					}
				}
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

	void StartFuse(uint clientId, uint fuseId)
	{
		HkLightFuse((IObjRW*)clientCloakStats[clientId].shipInspect, fuseId, 0.0f, 0.0f, 0.0f);
	}

	void StopFuse(uint clientId, uint fuseId)
	{
		HkUnLightFuse((IObjRW*)clientCloakStats[clientId].shipInspect, fuseId);
	}

	void SendEquipmentActivationState(uint clientId, uint cargoId, bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = clientCloakStats[clientId].ship->iID;
		activateEquipment.sID = cargoId;
		Server.ActivateEquip(clientId, activateEquipment);
		HookClient->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
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
					else if (cargo.iArchID == cloakDefinitions[index].powerArchetypeId)
					{
						clientStats.powerId = cargo.iID;
					}
					else if (cargo.iArchID == clientStats.effectsDefinition->cloakingDeviceArchetypeId)
					{
						clientStats.cloakCargoId = cargo.iID;
					}
				}

				// Do not check for Activator. Ships that have it shot-off but didn't dock yet will otherwise never uncloak again after login.
				if (clientStats.cloakCargoId && clientStats.powerId)
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

		uint shipId;
		pub::Player::GetShip(clientId, shipId);
		clientStats.ship = (CShip*)CObject::Find(shipId, CObject::CSHIP_OBJECT);
		clientStats.shipInspect = HkGetInspect(clientId);

		if (!clientStats.ship || !clientStats.shipInspect)
			return;

		float currentShieldCapacity;
		float maxShieldCapacity;
		bool shieldsUp;
		pub::SpaceObj::GetShieldHealth(shipId, currentShieldCapacity, maxShieldCapacity, shieldsUp);
		clientStats.shieldPresent = maxShieldCapacity > 0.0f;

		clientCloakStats.insert({ clientId, clientStats });

		// Deactivate the power initially. This must be done here as there's additional information used in the called function.
		SendEquipmentActivationState(clientId, clientStats.powerId, false);
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

	void SynchronizeShieldStateWithCloakState(uint clientId)
	{
		if (clientCloakStats[clientId].shieldPresent)
			SendEquipmentActivationState(clientId, SHIELD_SLOT_ID, IsFullyUncloaked(clientId));
	}

	void SynchronizePowerStateWithCloakState(uint clientId)
	{
		SendEquipmentActivationState(clientId, clientCloakStats[clientId].powerId, clientCloakStats[clientId].cloakState == CloakState::Cloaked);
	}

	CloakReturnState TryCloak(uint clientId)
	{
		CloakReturnState result = CloakReturnState::None;

		const auto cloakState = clientCloakStats[clientId].cloakState;
		if (!clientCloakStats[clientId].initialUncloakCompleted)
		{
			result = CloakReturnState::NotInitialized;
		}
		else if (!clientCloakStats[clientId].activatorCargoId)
		{
			result = CloakReturnState::Destroyed;
		}
		else if (cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking)
		{
			result = CloakReturnState::Successful;
		}
		else if (clientCloakStats[clientId].dockingManeuverActive || ClientInfo[clientId].bTradelane)
		{
			result = CloakReturnState::DockSequence;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}
		else if (clientCloakStats[clientId].insideNoCloakZone)
		{
			result = CloakReturnState::Blocked;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}
		else if (timeInMS() - clientCloakStats[clientId].uncloakTimeStamp < clientCloakStats[clientId].effectsDefinition->uncloakDuration)
		{
			result = CloakReturnState::NotReady;
			pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
		}

		if (result == CloakReturnState::None)
		{
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].cloakCargoId, true);
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].activatorCargoId, true);
			StartFuse(clientId, clientCloakStats[clientId].effectsDefinition->cloakFuseId);
			clientCloakStats[clientId].cloakTimeStamp = timeInMS();
			clientCloakStats[clientId].cloakState = CloakState::Cloaking;
			SynchronizeWeaponGroupsWithCloakState(clientId);
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

		if (timeInMS() - clientCloakStats[clientId].cloakTimeStamp > clientCloakStats[clientId].effectsDefinition->cloakDuration)
		{
			clientIdsRequestingUncloak.erase(clientId);
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].cloakCargoId, false);
			if (clientCloakStats[clientId].activatorCargoId)
				SendEquipmentActivationState(clientId, clientCloakStats[clientId].activatorCargoId, false);
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].powerId, false);
			if (clientCloakStats[clientId].initialUncloakCompleted)
			{
				StartFuse(clientId, clientCloakStats[clientId].effectsDefinition->uncloakFuseId);
				pub::Player::SendNNMessage(clientId, pub::GetNicknameId("deactivated"));
			}
			clientCloakStats[clientId].uncloakTimeStamp = timeInMS();
			clientCloakStats[clientId].cloakState = CloakState::Uncloaking;
			SynchronizeWeaponGroupsWithCloakState(clientId);
			return true;
		}
		return false;
	}

	void QueueUncloak(uint clientId)
	{
		if (!IsValidCloakableClient(clientId) || !clientCloakStats[clientId].initialUncloakCompleted)
			return;

		if (!TryUncloak(clientId))
			clientIdsRequestingUncloak.insert(clientId);
	}

	void AttemptInitialUncloak(uint clientId)
	{
		if (IsValidCloakableClient(clientId) && !clientCloakStats[clientId].initialUncloakCompleted && clientCloakStats[clientId].cloakState == CloakState::Cloaked)
			TryUncloak(clientId);
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
			cloakState = currentTime - clientCloakStats[clientId].cloakTimeStamp > clientCloakStats[clientId].effectsDefinition->cloakDuration ? CloakState::Cloaked : CloakState::Cloaking;
		}
		else if (cloakState == CloakState::Uncloaking)
		{
			cloakState = currentTime - clientCloakStats[clientId].uncloakTimeStamp > clientCloakStats[clientId].effectsDefinition->uncloakDuration ? CloakState::Uncloaked : CloakState::Uncloaking;
			if (!clientCloakStats[clientId].initialUncloakCompleted && cloakState == CloakState::Uncloaked)
				clientCloakStats[clientId].initialUncloakCompleted = true;
		}
		if (cloakState == CloakState::Cloaked || cloakState == CloakState::Uncloaked)
		{
			StopFuse(clientId, clientCloakStats[clientId].effectsDefinition->cloakFuseId);
			StopFuse(clientId, clientCloakStats[clientId].effectsDefinition->uncloakFuseId);
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
			if (!IsValidCloakableClient(clientId))
				continue;

			if (!clientCloakStats[clientId].activatorCargoId || !HasMountedEquipmentByCargoId(clientId, clientCloakStats[clientId].activatorCargoId))
			{
				clientCloakStats[clientId].activatorCargoId = 0;
				QueueUncloak(clientId);
			}

			const auto& cloakState = clientCloakStats[clientId].cloakState;

			// Synchronize cloak state to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].cloakCargoId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);
			// Synchronize activator state to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].activatorCargoId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Uncloak player in no-cloak-zones
			CheckPlayerInNoCloakZones(clientId, playerData->iSystemID, playerData->iShipID);

			// Cloak state update when effect time has passed
			// This also sets the initial uncloaked flag.
			UpdateStateByEffectTimings(clientId, now);

			// The rest in the update loop should be ignored for not initially uncloaked ships.
			if (!clientCloakStats[clientId].initialUncloakCompleted)
				continue;

			SynchronizeShieldStateWithCloakState(clientId);
			SynchronizePowerStateWithCloakState(clientId);

			if (cloakState == CloakState::Cloaked && clientCloakStats[clientId].ship->get_power() <= clientCloakStats[clientId].statsDefinition->powerUsage)
				QueueUncloak(clientId);

			// Schedule cloak changes
			if (clientIdsRequestingUncloak.contains(clientId))
				QueueUncloak(clientId);
		}
		lastSynchronizeTimeStamp = now;
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

				case CloakReturnState::Successful:
					successful = true;
					break;
			}
		}
		else
		{
			successful = TryUncloak(clientId);
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
				SendEquipmentActivationState(clientId, clientCloakStats[clientId].activatorCargoId, activate);

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
			RemoveCloakingDevicesAndPower(clientId);
		}
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			if (!EquipCloakingDeviceAndPower(clientId))
				RemoveCloakingDevicesAndPower(clientId);
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