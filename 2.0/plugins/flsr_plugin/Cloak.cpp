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
 * cloak_transition_prolongation = 1
 * jump_gate_decloak_radius = 2000
 * jump_hole_decloak_radius = 1000
 * instant_cloaking_device_nickname = 
 * 
 * [Ship]
 * ship_nickname = 
 * cloaking_device_nickname = 
 * cloaking_device_hardpoint = 
 * cloak_fuse_name = 
 * uncloak_fuse_name = 
 * 
 * [CloakActivators]
 * activator_nickname = 
 * 
 * 
 * General Functionality:
 * 
 * * `instant_cloaking_device_nickname` is a cloaking device with zero effect duration. Used to properly synchronize cloaking effects with players.
 * 
 * * `activator_name` is a proxy-equipment which acts as way to enable/disable the cloaking device. Ideally this is a launcher/gun to show in the weapons list.
 *    Activation can be done by either using the hot-keys for enable/disable weapon, or by firing it (works only to enable, as cloak blocks afterwards to disable it).
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
 * * `cloaking_device_hardpoint` is the name of the hardpont where the Cloaking Device will be mounted to. Ideally this should be inside the ship and parented to the main hull.
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
	const uint FIRE_DRY_ID = CreateID("fire_dry");
	const uint WARNING_NN_ID = pub::GetNicknameId("warning");
	const uint JUMP_GATE_NN_ID = pub::GetNicknameId("object_jump_gate");
	const uint JUMP_HOLE_NN_ID = pub::GetNicknameId("object_jump_hole");
	const uint SHIELD_SLOT_ID = 65521;
	// The main update loop's interval.
	const uint TIMER_INTERVAL = 100;

	// When a player joins when another player is in cloak-transition, timings get confused.
	// To counter this, a Cloaking Device with zero-time is used to completely cloak/uncloak at the end of the transitions to make sur the state is always where it should be.
	static uint instantCloakingDeviceArchetypeId = 0;
	// When toggling cloak extremely fast, small lags can cause confusion of state at the client. Cloak/uncloak durations can be prolonged to reduce this effect.
	static uint cloakTransitionProlongation = 1;
	static float jumpGateDecloakRadius = 2000.0f;
	static float jumpHoleDecloakRadius = 1000.0f;

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

	struct ClientCloakStats
	{
		CShip* ship = 0;
		IObjInspectImpl* shipInspect = 0;
		uint activatorCargoId = 0;
		uint activatorArchetypeId = 0;
		std::string activatorHardpoint = "";
		uint cloakCargoId = 0;
		uint instantCloakCargoId = 0;
		uint engineCargoId = 0;
		bool engineKillActive = false;
		std::vector<uint> powerCargoIds;
		bool shieldPresent = false;
		CloakState cloakState = CloakState::Cloaked;
		bool initialUncloakCompleted = false;
		mstime cloakTimeStamp = 0;
		mstime uncloakTimeStamp = 0;
		mstime lastDrySoundTimeStamp = 0;
		bool insideNoCloakZone = false;
		bool dockingManeuverActive = false;
		ShipEffectDefinition* effectsDefinition = 0;
	};

	enum class CloakReturnState
	{
		None,
		Successful,
		DockSequence,
		Blocked,
		Destroyed,
		NotReady,
		NotInitialized
	};

	enum class UncloakReason
	{
		Initial,
		User,
		Destroyed,
		Power,
		Docking,
		JumpGate,
		JumpHole,
		Anomaly,
		Disrupted
	};

	static std::set<uint> cloakActivatorArchetypeIds;
	static std::vector<ShipEffectDefinition> shipEffects;
	static std::map<uint, ClientCloakStats> clientCloakStats;

	static std::map<uint, std::vector<Vector>> jumpGatePositionsPerSystem;
	static std::map<uint, std::vector<Vector>> jumpHolePositionsPerSystem;

	static std::map<uint, UncloakReason> clientIdsRequestingUncloak;

	static std::map<uint, std::vector<uint>> clientCloakScanners;
	static std::map<uint, std::vector<uint>> clientCloakDisruptors;

	bool IsValidCloakableClient(const uint clientId)
	{
		if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId) && clientCloakStats.contains(clientId))
		{
			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			return shipId;
		}
		return false;
	}

	bool IsFullyUncloaked(const uint clientId)
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
				Vector position;
				Matrix orientation;
				pub::SpaceObj::GetLocation(solar->iID, position, orientation);
				jumpGatePositionsPerSystem[solar->iSystem].push_back(position);
			}
			else if (type == OBJ_JUMP_HOLE && jumpHoleDecloakRadius > 0.0f && solar->GetParentNickname().IsEmpty())
			{
				Vector position;
				Matrix orientation;
				pub::SpaceObj::GetLocation(solar->iID, position, orientation);
				jumpHolePositionsPerSystem[solar->iSystem].push_back(position);
			}
		}
	}

	void ClearClientData(const uint clientId)
	{
		if (clientCloakStats[clientId].ship)
		{
			Mark::ShowObjectMark(clientCloakStats[clientId].ship->iID);
			clientCloakStats[clientId].ship->Release();
		}
		clientCloakStats.erase(clientId);
		clientIdsRequestingUncloak.erase(clientId);
	}

	void LoadCloakSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + Globals::CLOAK_CONFIG_FILE;

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
						else if (ini.is_value("cloak_transition_prolongation"))
							cloakTransitionProlongation = ini.get_value_float(0) * 1000;
						else if (ini.is_value("instant_cloaking_device_nickname"))
							instantCloakingDeviceArchetypeId = CreateID(ini.get_value_string(0));
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

				if (ini.is_header("CloakActivators"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("activator_nickname"))
							cloakActivatorArchetypeIds.insert(CreateID(ini.get_value_string(0)));
					}
				}
			}
			ini.close();
		}
	}

	static bool initialized = false;

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
			shipEffect.cloakDuration = archetype->fCloakinTime * 1000 + cloakTransitionProlongation;
			shipEffect.uncloakDuration = archetype->fCloakoutTime * 1000 + cloakTransitionProlongation;
		}

		CollectAllJumpSolarsPerSystem();
	}

	std::list<CARGO_INFO> GetClientCargoList(uint clientId)
	{
		std::list<CARGO_INFO> cargoList;
		if (!HkIsValidClientID(clientId))
			return cargoList;

		const std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		int remainingCargoHoldSize;
		HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize);
		return cargoList;
	}

	bool EquipEquipment(const uint clientId, const uint archetypeId, const std::string& hardpoint)
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

	bool EquipCloakingDevices(const uint clientId)
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

		const std::list<CARGO_INFO>& cargoList = GetClientCargoList(clientId);
		for (const uint activatorArchetypeId : cloakActivatorArchetypeIds)
		{
			for (const auto& cargo : cargoList)
			{
				if (cargo.bMounted && cargo.iArchID == activatorArchetypeId)
				{
					return EquipEquipment(clientId, cloakingDeviceArchetypeId, hardpoint) &&
						   EquipEquipment(clientId, instantCloakingDeviceArchetypeId, hardpoint);
				}
			}
		}

		return false;
	}

	void RemoveCloakingDevices(const uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		const std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		const std::list<CARGO_INFO>& cargoList = GetClientCargoList(clientId);
		for (const auto& cargo : cargoList)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(cargo.iArchID);
			if (equipment && equipment->get_class_type() == Archetype::AClassType::CLOAKING_DEVICE)
				HkRemoveCargo(characterNameWS, cargo.iID, 1);
		}
	}

	bool HasMountedEquipmentByCargoId(const uint clientId, const uint cargoId)
	{
		if (!HkIsValidClientID(clientId))
			return false;

		const std::list<CARGO_INFO>& cargoList = GetClientCargoList(clientId);
		for (const auto& cargo : cargoList)
		{
			if (cargo.bMounted && cargo.iID == cargoId)
				return true;
		}
		return false;
	}

	void StartFuse(const uint clientId, const uint fuseId)
	{
		HkLightFuse((IObjRW*)clientCloakStats[clientId].shipInspect, fuseId, 0.0f, 0.0f, 0.0f);
	}

	void StopFuse(const uint clientId, const uint fuseId)
	{
		HkUnLightFuse((IObjRW*)clientCloakStats[clientId].shipInspect, fuseId);
	}

	void SendEquipmentActivationState(const uint clientId, const uint cargoId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = clientCloakStats[clientId].ship->iID;
		activateEquipment.sID = cargoId;
		Server.ActivateEquip(clientId, activateEquipment);
		HookClient->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	void InstallCloak(const uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		ClientCloakStats clientStats;

		uint shipArchetypeId;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		for (size_t index = 0; index < shipEffects.size(); index++)
		{
			if (shipEffects[index].shipArchetypeId == shipArchetypeId)
			{
				clientStats.effectsDefinition = &shipEffects[index];
				break;
			}
		}
		if (!clientStats.effectsDefinition)
			return;

		const std::list<CARGO_INFO>& cargoList = GetClientCargoList(clientId);
		for (const auto& cargo : cargoList)
		{
			if (cargo.bMounted)
			{
				if (cargo.iArchID == clientStats.effectsDefinition->cloakingDeviceArchetypeId)
				{
					clientStats.cloakCargoId = cargo.iID;
				}
				else if (cargo.iArchID == instantCloakingDeviceArchetypeId)
				{
					clientStats.instantCloakCargoId = cargo.iID;
				}

				if (clientStats.cloakCargoId && clientStats.instantCloakCargoId)
					break;
			}
		}

		if (!clientStats.cloakCargoId || !clientStats.instantCloakCargoId)
			return;

		for (const uint activatorArchetypeId : cloakActivatorArchetypeIds)
		{
			for (const auto& cargo : cargoList)
			{
				if (cargo.bMounted && cargo.iArchID == activatorArchetypeId)
				{
					clientStats.activatorArchetypeId = activatorArchetypeId;
					clientStats.activatorCargoId = cargo.iID;
					clientStats.activatorHardpoint = std::string(cargo.hardpoint.value);
					break;
				}
			}
			if (clientStats.activatorArchetypeId)
				break;
		}
		if (!clientStats.activatorArchetypeId)
			return;

		// Find all other power plants and the engine
		for (const auto& cargo : cargoList)
		{
			if (cargo.bMounted &&
				cargo.iArchID != clientStats.activatorArchetypeId &&
				cargo.iArchID != clientStats.effectsDefinition->cloakingDeviceArchetypeId &&
				cargo.iArchID != instantCloakingDeviceArchetypeId)
			{
				const Archetype::Equipment* equipment = Archetype::GetEquipment(cargo.iArchID);
				if (!equipment)
					continue;

				if (equipment->get_class_type() == Archetype::AClassType::POWER)
					clientStats.powerCargoIds.push_back(cargo.iID);
				else if (equipment->get_class_type() == Archetype::AClassType::ENGINE)
					clientStats.engineCargoId = cargo.iID;
			}
		}

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
	}

	void SynchronizeWeaponGroupsWithCloakState(const uint clientId)
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

	void SynchronizeShieldStateWithCloakState(const uint clientId)
	{
		if (clientCloakStats[clientId].shieldPresent)
			SendEquipmentActivationState(clientId, SHIELD_SLOT_ID, IsFullyUncloaked(clientId));
	}

	void SynchronizePowerStateWithCloakState(const uint clientId)
	{
		bool cloakPowerDrainActive = clientCloakStats[clientId].cloakState == CloakState::Cloaked && !clientIdsRequestingUncloak.contains(clientId);
		bool powerPlantsActive = IsFullyUncloaked(clientId);
		for (const uint cargoId : clientCloakStats[clientId].powerCargoIds)
			SendEquipmentActivationState(clientId, cargoId, powerPlantsActive);
	}

	void PlayDrySound(const uint clientId)
	{
		const mstime now = timeInMS();
		if (now - clientCloakStats[clientId].lastDrySoundTimeStamp > 1000)
		{
			clientCloakStats[clientId].lastDrySoundTimeStamp = timeInMS();
			pub::Audio::PlaySoundEffect(clientId, FIRE_DRY_ID);
		}
	}

	CloakReturnState TryCloak(const uint clientId)
	{
		CloakReturnState result = CloakReturnState::None;

		const CloakState cloakState = clientCloakStats[clientId].cloakState;
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
			PlayDrySound(clientId);
		}
		else if (clientCloakStats[clientId].insideNoCloakZone)
		{
			result = CloakReturnState::Blocked;
			PlayDrySound(clientId);
		}
		else if (timeInMS() - clientCloakStats[clientId].uncloakTimeStamp < clientCloakStats[clientId].effectsDefinition->uncloakDuration)
		{
			result = CloakReturnState::NotReady;
			PlayDrySound(clientId);
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

	bool TryUncloak(const uint clientId, const UncloakReason reason)
	{
		const CloakState cloakState = clientCloakStats[clientId].cloakState;
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
			if (clientCloakStats[clientId].initialUncloakCompleted)
			{
				StartFuse(clientId, clientCloakStats[clientId].effectsDefinition->uncloakFuseId);
				switch (reason)
				{
					case UncloakReason::JumpGate:
						pub::Player::SendNNMessage(clientId, WARNING_NN_ID);
						pub::Player::SendNNMessage(clientId, JUMP_GATE_NN_ID);
						break;

					case UncloakReason::JumpHole:
						pub::Player::SendNNMessage(clientId, WARNING_NN_ID);
						pub::Player::SendNNMessage(clientId, JUMP_HOLE_NN_ID);
						break;

					default:
						break;
				}
			}
			clientCloakStats[clientId].uncloakTimeStamp = timeInMS();
			clientCloakStats[clientId].cloakState = CloakState::Uncloaking;
			SynchronizeWeaponGroupsWithCloakState(clientId);
			return true;
		}
		return false;
	}

	void QueueUncloak(const uint clientId, const UncloakReason reason)
	{
		if (!IsValidCloakableClient(clientId) || !clientCloakStats[clientId].initialUncloakCompleted)
			return;

		// Try to uncloak. If it fails, queue the attempt. Do not override already existing uncloak reasons.
		if (!TryUncloak(clientId, reason) && !clientIdsRequestingUncloak.contains(clientId))
			clientIdsRequestingUncloak[clientId] = reason;
	}

	void AttemptInitialUncloak(uint clientId)
	{
		if (IsValidCloakableClient(clientId) && !clientCloakStats[clientId].initialUncloakCompleted && clientCloakStats[clientId].cloakState == CloakState::Cloaked)
			TryUncloak(clientId, UncloakReason::Initial);
	}

	bool CheckDockCall(const uint ship, const uint dockTargetId, const uint dockPortIndex, const DOCK_HOST_RESPONSE response)
	{
		const uint clientId = HkGetClientIDByShip(ship);
		if (!IsValidCloakableClient(clientId))
			return true;

		clientCloakStats[clientId].dockingManeuverActive = false;

		// dockPortIndex == -1 -> aborting the dock
		if (!dockTargetId || dockPortIndex == -1 || response == DOCK_HOST_RESPONSE::ACCESS_DENIED || response == DOCK_HOST_RESPONSE::DOCK_DENIED)
			return true;

		QueueUncloak(clientId, UncloakReason::Docking);
		clientCloakStats[clientId].dockingManeuverActive = true;
		return true;
	}

	void CheckPlayerInNoCloakZones(const uint clientId, const uint clientSystemId, const uint clientShipId)
	{
		bool insideNoCloakZone = false;
		UncloakReason uncloakReason;
		Vector shipPosition;
		Matrix shipOrientation;
		pub::SpaceObj::GetLocation(clientShipId, shipPosition, shipOrientation);
		if (jumpGatePositionsPerSystem.contains(clientSystemId))
		{
			for (const Vector& jumpGatePosition : jumpGatePositionsPerSystem[clientSystemId])
			{
				if (HkDistance3D(jumpGatePosition, shipPosition) < jumpGateDecloakRadius)
				{
					insideNoCloakZone = true;
					uncloakReason = UncloakReason::JumpGate;
					break;
				}
			}
		}
		if (!insideNoCloakZone && jumpHolePositionsPerSystem.contains(clientSystemId))
		{
			for (const Vector& jumpHolePosition : jumpHolePositionsPerSystem[clientSystemId])
			{
				if (HkDistance3D(jumpHolePosition, shipPosition) < jumpHoleDecloakRadius)
				{
					insideNoCloakZone = true;
					uncloakReason = UncloakReason::JumpHole;
					break;
				}
			}
		}

		clientCloakStats[clientId].insideNoCloakZone = insideNoCloakZone;
		const CloakState cloakState = clientCloakStats[clientId].cloakState;
		if (insideNoCloakZone && (cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking))
			QueueUncloak(clientId, uncloakReason);
	}

	void UpdateStateByEffectTimings(const uint clientId, const mstime currentTime)
	{
		CloakState& cloakState = clientCloakStats[clientId].cloakState;
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

	void FireCloakActivator(const uint clientId)
	{
		if (IsValidCloakableClient(clientId) && clientCloakStats[clientId].activatorCargoId)
		{
			XFireWeaponInfo info;
			info.iObject = clientCloakStats[clientId].ship->iID;
			ushort slot[2] = { (ushort)clientCloakStats[clientId].activatorCargoId, 0 };
			info.sHpIdsBegin = &slot[0];
			info.sHpIdsLast = &slot[1];
			info.sHpIdsEnd = &slot[1];
			info.iDunno = 0;
			info.vTarget.x = 1;
			info.vTarget.y = 0;
			info.vTarget.z = 0;
			Server.FireWeapon(clientId, info);
			HookClient->Send_FLPACKET_COMMON_FIREWEAPON(clientId, info);
		}
	}

	static mstime lastSynchronizeTimeStamp = 0;

	// This is executed to make sure players that spawn into a system with other cloaking players have their visibility synced.
	void UpdateCloakClients()
	{
		if (!Modules::GetModuleState("CloakModule"))
			return;

		if (lastSynchronizeTimeStamp == 0)
			lastSynchronizeTimeStamp = timeInMS();

		const mstime now = timeInMS();

		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			const uint clientId = HkGetClientIdFromPD(playerData);
			if (!IsValidCloakableClient(clientId))
				continue;

			if (!clientCloakStats[clientId].activatorCargoId || !HasMountedEquipmentByCargoId(clientId, clientCloakStats[clientId].activatorCargoId))
			{
				clientCloakStats[clientId].activatorCargoId = 0;
				QueueUncloak(clientId, UncloakReason::Destroyed);
			}

			const CloakState& cloakState = clientCloakStats[clientId].cloakState;

			// Synchronize cloak state to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].cloakCargoId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);
			// Synchronize fallback instant cloak to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].instantCloakCargoId, cloakState == CloakState::Cloaked || cloakState == CloakState::Uncloaking);

			// Synchronize activator state to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].activatorCargoId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Uncloak player in no-cloak-zones
			CheckPlayerInNoCloakZones(clientId, playerData->iSystemID, playerData->iShipID);

			const CloakState oldCloakState = cloakState;
			// Cloak state update when effect time has passed
			// This also sets the initial uncloaked flag.
			UpdateStateByEffectTimings(clientId, now);

			// Make sure to remove this player from everyone's mark list when the cloak state changed.
			if (oldCloakState != cloakState)
			{
				if (cloakState == CloakState::Cloaked)
					Mark::HideObjectMark(clientCloakStats[clientId].ship->iID);
				else
					Mark::ShowObjectMark(clientCloakStats[clientId].ship->iID);
			}

			// The rest in the update loop should be ignored for not initially uncloaked ships.
			if (!clientCloakStats[clientId].initialUncloakCompleted)
				continue;

			SynchronizeShieldStateWithCloakState(clientId);
			SynchronizePowerStateWithCloakState(clientId);

			// Uncloak when power is empty.
			if (cloakState == CloakState::Cloaked && clientCloakStats[clientId].ship->get_power() <= 0.0f)
				QueueUncloak(clientId, UncloakReason::Power);

			// Schedule cloak changes.
			if (clientIdsRequestingUncloak.contains(clientId))
				QueueUncloak(clientId, clientIdsRequestingUncloak[clientId]);

			// Consume energy.
			if (cloakState == CloakState::Cloaked)
				FireCloakActivator(clientId);
		}
		lastSynchronizeTimeStamp = now;
	}

	std::vector<uint> FindMountedEquipments(const std::list<CARGO_INFO>& cargoList, const uint archetypeId)
	{
		std::vector<uint> result;
		for (const auto& cargo : cargoList)
		{
			if (cargo.bMounted && cargo.iArchID == archetypeId)
				result.push_back(cargo.iID);
		}
		return result;
	}

	CloakState GetClientCloakState(const uint clientId)
	{
		if (clientCloakStats.contains(clientId))
			return clientCloakStats[clientId].cloakState;
		return CloakState::Uncloaked;
	}

	CloakState FindShipCloakState(const uint shipId)
	{
		for (const auto& cloakStats: clientCloakStats)
		{
			if (cloakStats.second.ship && cloakStats.second.ship->iID == shipId)
				return cloakStats.second.cloakState;
		}
		return CloakState::Uncloaked;
	}

	bool ToggleClientCloakActivator(const uint clientId, const bool active)
	{
		bool successful = false;
		if (active)
		{
			switch (TryCloak(clientId))
			{
				case CloakReturnState::Successful:
					successful = true;
					break;
			}
		}
		else
		{
			successful = TryUncloak(clientId, UncloakReason::User);
		}
		return successful;
	}

	void __stdcall ActivateEquip(unsigned int clientId, XActivateEquip const& activateEquip)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
		{
			if (clientCloakStats[clientId].activatorCargoId == activateEquip.sID)
				ToggleClientCloakActivator(clientId, activateEquip.bActivate);

			if (clientCloakStats[clientId].engineCargoId == activateEquip.sID)
				clientCloakStats[clientId].engineKillActive = !activateEquip.bActivate;
		}
	}

	void __stdcall FireWeapon(unsigned int clientId, XFireWeaponInfo const& fireWeaponInfo)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
		{
			const size_t size = ((size_t)fireWeaponInfo.sHpIdsLast - (size_t)fireWeaponInfo.sHpIdsBegin) / 2;
			for (size_t index = 0; index < size; index++)
			{
				if (fireWeaponInfo.sHpIdsBegin[index] == clientCloakStats[clientId].activatorCargoId)
				{
					if (clientCloakStats[clientId].cloakState == CloakState::Uncloaked && ToggleClientCloakActivator(clientId, true))
						SendEquipmentActivationState(clientId, clientCloakStats[clientId].activatorCargoId, true);
					break;
				}
			}
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

			QueueUncloak(clientId, UncloakReason::User);

			if (IsValidCloakableClient(clientId))
				clientCloakStats[clientId].dockingManeuverActive = false;
		}
	}

	void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			ClearClientData(clientId);
			InstallCloak(clientId);
		}
	}
	
	int __cdecl Dock_Call(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && !CheckDockCall(ship, dockTargetId, dockPortIndex, response))
			returncode = NOFUNCTIONCALL;

		return 0;
	}

	void __stdcall GoTradelane(unsigned int clientId, struct XGoTradelane const& goToTradelane)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
			QueueUncloak(clientId, UncloakReason::Docking);
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

		if (Modules::GetModuleState("CloakModule") && !EquipCloakingDevices(clientId))
			RemoveCloakingDevices(clientId);
	}

	void SetServerSideEngineState(const uint clientId, const bool active)
	{
		if (IsValidCloakableClient(clientId) && clientCloakStats[clientId].engineCargoId)
		{
			XActivateEquip activateEquipment;
			activateEquipment.bActivate = active;
			activateEquipment.iSpaceID = clientCloakStats[clientId].ship->iID;
			activateEquipment.sID = clientCloakStats[clientId].engineCargoId;
			Server.ActivateEquip(clientId, activateEquipment);
			clientCloakStats[clientId].engineKillActive = !active;
		}
	}

	void __stdcall SPObjUpdate(SSPObjUpdateInfo const& updateInfo, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			AttemptInitialUncloak(clientId);

			// Fix bug of Throttle on server not being correctly set.
			if (IsValidCloakableClient(clientId))
			{
				clientCloakStats[clientId].ship->set_throttle(updateInfo.fThrottle);

				// Setting Throttle on the ship makes the server think that engine was turned on again.
				if (clientCloakStats[clientId].engineKillActive)
					SetServerSideEngineState(clientId, false);
			}
		}
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
			ClearClientData(clientId);
	}

	void __stdcall ShipDestroyed(DamageList* dmg, DWORD* ecx, uint killed)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule"))
		{
			if (!killed)
				return;
			const CShip* ship = (CShip*)ecx[4];
			if (!ship)
				return;
			const uint clientId = ship->GetOwnerPlayer();
			if (!clientId)
				return;
			ClearClientData(clientId);
		}
	}

	void UserCmd_CLOAK(uint clientId, const std::wstring& wscParam)
	{
		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
		{
			const CloakState cloakState = clientCloakStats[clientId].cloakState;
			ToggleClientCloakActivator(clientId, cloakState == CloakState::Uncloaked || cloakState == CloakState::Uncloaking);
		}
	}

	void UserCmd_UNCLOAK(uint clientId, const std::wstring& wscParam)
	{
		if (Modules::GetModuleState("CloakModule") && IsValidCloakableClient(clientId))
			ToggleClientCloakActivator(clientId, false);
	}

	/**
	 * This fixes a Vanilla Server Bug.
	 * The Server is not receiving an information about Engine Kill being disabled when Cruise is activated by the Client.
	 * So while Engine Kill is active, and Cruise gets activated, the Server thinks the Engine is off and does not draw any Cruise Power.
	 * This makes Server-Side power state of the Client's ship become asynchronous with the Client's own power state.
	 * To fix this, the Server enables the Engine again once Cruise goes online. The Client does this anyway.
	 */
	void __stdcall ActivateCruise(unsigned int clientId, struct XActivateCruise const& activateCruise)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && activateCruise.bActivate)
			SetServerSideEngineState(clientId, true);
	}
}