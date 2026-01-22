#include "Cloak.h"
#include "Mark.h"
#include "Plugin.h"

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
 * [NoCloakAreas]
 * position = systemNickname, x, y, z, radius, voice_message
 * object = nickname, radius, voice_message
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
	const uint SHIELD_SLOT_ID = 65521;
	// The main update loop's interval.
	const uint TIMER_INTERVAL = 100;
	const float LOW_ENERGY_THRESHOLD = 100.0f;
	const mstime DRY_SOUND_DEBOUNCE_TIME = 1000;
	const mstime NEURAL_NET_DEBOUNCE_TIME = 2000;

	// When a player joins when another player is in cloak-transition, timings get confused.
	// To counter this, a Cloaking Device with zero-time is used to completely cloak/uncloak at the end of the transitions to make sur the state is always where it should be.
	uint instantCloakingDeviceArchetypeId = 0;
	// When toggling cloak extremely fast, small lags can cause confusion of state at the client. Cloak/uncloak durations can be prolonged to reduce this effect.
	uint cloakTransitionProlongation = 1;
	float jumpGateDecloakRadius = 2000.0f;
	float jumpHoleDecloakRadius = 1000.0f;

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
		uint shipId = 0;
		IObjRW* shipInspect = 0;
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
		mstime lastNeuralNetSoundTimeStamp = 0;
		boolean insideNoCloakArea = false;
		uint currentNoCloakAreaNNVoiceMessageId = 0;
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
		NoCloakArea,
		Disrupted
	};

	std::set<uint> cloakActivatorArchetypeIds;
	std::vector<ShipEffectDefinition> shipEffects;
	std::unordered_map<uint, ClientCloakStats> clientCloakStats;
	std::unordered_map<uint, UncloakReason> clientIdsRequestingUncloak;

	struct NoCloakArea
	{
		uint objectId = 0;
		Vector position;
		float radius;
		uint NNVoiceMessageId;
	};
	std::unordered_map<uint, std::vector<NoCloakArea>> noCloakAreasPerSystem;
	std::unordered_map<uint, NoCloakArea> noCloakObjectDefinitionsByNicknameId;
	std::vector<NoCloakArea> unprocessedObjectNoCloakAreas;

	bool static IsValidCloakableClient(const uint clientId)
	{
		if (HkIsValidClientID(clientId) && !HkIsInCharSelectMenu(clientId) && clientCloakStats.contains(clientId))
		{
			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			return shipId;
		}
		return false;
	}

	static bool IsFullyUncloaked(const uint clientId)
	{
		return IsValidCloakableClient(clientId) && clientCloakStats[clientId].cloakState == CloakState::Uncloaked;
	}

	bool TryRegisterNoCloakSolar(const std::string& nickname, uint objectId)
	{
		const uint nicknameId = CreateID(nickname.c_str());
		IObjRW* inspect;
		StarSystem* starSystem;
		if (noCloakObjectDefinitionsByNicknameId.contains(nicknameId) && GetShipInspect(objectId, inspect, starSystem))
		{
			const CSimple* object = inspect->cobj;
			NoCloakArea area = noCloakObjectDefinitionsByNicknameId[nicknameId];
			area.objectId = objectId;
			area.position = object->get_position();
			noCloakAreasPerSystem[object->system].push_back(area);
			return true;
		}
		return false;
	}

	static void CollectNoCloakObjectsPerSystem()
	{
		const uint JUMP_GATE_NN_ID = pub::GetNicknameId("object_jump_gate");
		const uint JUMP_HOLE_NN_ID = pub::GetNicknameId("object_jump_hole");
		CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
		while (solar != NULL)
		{
			for (NoCloakArea& objectArea : unprocessedObjectNoCloakAreas)
			{
				if (objectArea.objectId == solar->get_id())
				{
					objectArea.position = solar->get_position();
					noCloakAreasPerSystem[solar->system].push_back(objectArea);
				}
			}

			// Jump Gates and Jump Holes are automatically added to the No Cloak Area list.
			// Jump objects with Parent are ignored. It is assumed those are special objects not meant to be used by players.
			const uint type = solar->get_type();
			if (type == ObjectType::JumpGate && jumpGateDecloakRadius > 0.0f && solar->GetParentNickname().IsEmpty())
			{
				NoCloakArea area;
				area.objectId = solar->get_id();
				area.position = solar->get_position();
				area.radius = jumpGateDecloakRadius;
				area.NNVoiceMessageId = JUMP_GATE_NN_ID;
				noCloakAreasPerSystem[solar->system].push_back(area);
			}
			else if (type == ObjectType::JumpHole && jumpHoleDecloakRadius > 0.0f && solar->GetParentNickname().IsEmpty())
			{
				NoCloakArea area;
				area.objectId = solar->get_id();
				area.position = solar->get_position();
				area.radius = jumpHoleDecloakRadius;
				area.NNVoiceMessageId = JUMP_HOLE_NN_ID;
				noCloakAreasPerSystem[solar->system].push_back(area);
			}
			solar = static_cast<CSolar*>(solar->FindNext());
		}
		unprocessedObjectNoCloakAreas.clear();
	}

	static void ClearClientData(const uint clientId)
	{
		Mark::RemoveCloakedPlayer(clientId);
		clientCloakStats.erase(clientId);
		clientIdsRequestingUncloak.erase(clientId);
	}

	static void LoadCloakSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-Cloak.cfg";

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
							cloakTransitionProlongation = static_cast<uint>(ini.get_value_float(0) * 1000.0f);
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
				
				if (ini.is_header("NoCloakAreas"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("position"))
						{
							const uint systemId = CreateID(ini.get_value_string(0));
							NoCloakArea area;
							area.position.x = ini.get_value_float(1);
							area.position.y = ini.get_value_float(2);
							area.position.z = ini.get_value_float(3);
							area.radius = ini.get_value_float(4);
							area.NNVoiceMessageId = CreateID(ini.get_value_string(5));
							noCloakAreasPerSystem[systemId].push_back(area);
						}

						if (ini.is_value("object"))
						{
							NoCloakArea area;
							area.objectId = CreateID(ini.get_value_string(0));
							area.position = { 0, 0, 0 };
							area.radius = ini.get_value_float(1);
							area.NNVoiceMessageId = CreateID(ini.get_value_string(2));
							noCloakObjectDefinitionsByNicknameId[area.objectId] = area;
							unprocessedObjectNoCloakAreas.push_back(area);
						}
					}
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

		ConPrint(L"Initializing Cloak... ");

		LoadCloakSettings();

		for (auto& shipEffect : shipEffects)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(shipEffect.cloakingDeviceArchetypeId);
			if (!equipment)
				continue;
			const Archetype::CloakingDevice* archetype = (Archetype::CloakingDevice*)equipment;
			shipEffect.cloakDuration = static_cast<int>(archetype->fCloakinTime * 1000.0f) + cloakTransitionProlongation;
			shipEffect.uncloakDuration = static_cast<int>(archetype->fCloakoutTime * 1000.0f) + cloakTransitionProlongation;
		}

		CollectNoCloakObjectsPerSystem();

		ConPrint(L"Done\n");
	}

	static std::list<CARGO_INFO> GetClientCargoList(uint clientId)
	{
		std::list<CARGO_INFO> cargoList;
		if (!HkIsValidClientID(clientId))
			return cargoList;

		const std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		int remainingCargoHoldSize;
		HkEnumCargo(characterNameWS, cargoList, remainingCargoHoldSize);
		return cargoList;
	}

	static bool EquipEquipment(const uint clientId, const uint archetypeId, const std::string& hardpoint)
	{
		return HkAddEquip(ARG_CLIENTID(clientId), archetypeId, hardpoint) == HKE_OK;
	}

	static bool EquipCloakingDevices(const uint clientId)
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

	static void RemoveCloakingDevices(const uint clientId)
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

	static bool HasMountedEquipmentByCargoId(const uint clientId, const uint cargoId)
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

	static void StartFuse(const uint clientId, const uint fuseId)
	{
		HkLightFuse(clientCloakStats[clientId].shipInspect, fuseId, 0.0f, 0.0f, 0.0f);
	}

	static void StopFuse(const uint clientId, const uint fuseId)
	{
		HkUnLightFuse(clientCloakStats[clientId].shipInspect, fuseId);
	}

	static void SendEquipmentActivationState(const uint clientId, const uint cargoId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = clientCloakStats[clientId].shipId;
		activateEquipment.sID = cargoId;
		Server.ActivateEquip(clientId, activateEquipment);
		HookClient->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	static void InstallCloak(const uint clientId)
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

		// Allow cloakCargoId = 0. It will be 0 when a ship's cloak was destroyed but not yet fully removed.
		clientStats.cloakCargoId = 0;
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

		if (!clientStats.instantCloakCargoId)
			return;

		// Allow activatorArchetypeId = 0. It will be 0 when a ship's cloak was destroyed but not yet fully removed.
		clientStats.activatorArchetypeId = 0;
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
		if (!shipId)
			return;

		clientStats.shipId = shipId;
		clientStats.shipInspect = HkGetInspect(clientId);
		if (!clientStats.shipInspect)
			return;

		float currentShieldCapacity;
		float maxShieldCapacity;
		bool shieldsUp;
		pub::SpaceObj::GetShieldHealth(shipId, currentShieldCapacity, maxShieldCapacity, shieldsUp);
		clientStats.shieldPresent = maxShieldCapacity > 0.0f;

		clientCloakStats.insert({ clientId, clientStats });
	}

	static void SynchronizeWeaponGroupsWithCloakState(const uint clientId)
	{
		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			if (HkGetClientIdFromPD(playerData) == clientId)
			{
				std::string currentWeaponGroups(playerData->weaponGroups.c_str());
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

	static void SynchronizeShieldStateWithCloakState(const uint clientId)
	{
		if (clientCloakStats[clientId].shieldPresent)
			SendEquipmentActivationState(clientId, SHIELD_SLOT_ID, IsFullyUncloaked(clientId));
	}

	static void SynchronizePowerStateWithCloakState(const uint clientId, const float currentShipPower)
	{
		const CloakState cloakState = clientCloakStats[clientId].cloakState;
		bool powerPlantsActive = (currentShipPower < LOW_ENERGY_THRESHOLD) || IsFullyUncloaked(clientId); // Make sure we can never drop to zero or negative energy. Or clients will crash.
		for (const uint cargoId : clientCloakStats[clientId].powerCargoIds)
			SendEquipmentActivationState(clientId, cargoId, powerPlantsActive);
	}

	static void PlayDrySound(const uint clientId)
	{
		const mstime now = timeInMS();
		if (now - clientCloakStats[clientId].lastDrySoundTimeStamp > DRY_SOUND_DEBOUNCE_TIME)
		{
			clientCloakStats[clientId].lastDrySoundTimeStamp = timeInMS();
			pub::Audio::PlaySoundEffect(clientId, FIRE_DRY_ID);
		}
	}

	static void PlayNeuralNetVoice(const uint clientId, const std::vector<uint> messageIds)
	{
		const mstime now = timeInMS();
		if (now - clientCloakStats[clientId].lastNeuralNetSoundTimeStamp > NEURAL_NET_DEBOUNCE_TIME)
		{
			clientCloakStats[clientId].lastNeuralNetSoundTimeStamp = timeInMS();
			for (const uint messageId : messageIds)
				pub::Player::SendNNMessage(clientId, messageId);
		}
	}

	static CloakReturnState TryCloak(const uint clientId)
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
		else if (bool inTradelane = false; clientCloakStats[clientId].dockingManeuverActive || (clientCloakStats[clientId].shipInspect->is_using_tradelane(&inTradelane) == 0 && inTradelane))
		{
			result = CloakReturnState::DockSequence;
			PlayDrySound(clientId);
		}
		else if (clientCloakStats[clientId].insideNoCloakArea)
		{
			result = CloakReturnState::Blocked;
			if (clientCloakStats[clientId].currentNoCloakAreaNNVoiceMessageId)
				PlayNeuralNetVoice(clientId, { WARNING_NN_ID, clientCloakStats[clientId].currentNoCloakAreaNNVoiceMessageId });
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

	static bool TryUncloak(const uint clientId, const UncloakReason reason)
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
				if (reason == UncloakReason::NoCloakArea && clientCloakStats[clientId].currentNoCloakAreaNNVoiceMessageId)
					PlayNeuralNetVoice(clientId, { WARNING_NN_ID, clientCloakStats[clientId].currentNoCloakAreaNNVoiceMessageId });
			}
			clientCloakStats[clientId].uncloakTimeStamp = timeInMS();
			clientCloakStats[clientId].cloakState = CloakState::Uncloaking;
			SynchronizeWeaponGroupsWithCloakState(clientId);
			return true;
		}
		return false;
	}

	static void QueueUncloak(const uint clientId, const UncloakReason reason)
	{
		if (!IsValidCloakableClient(clientId) || !clientCloakStats[clientId].initialUncloakCompleted)
			return;

		// Try to uncloak. If it fails, queue the attempt. Do not override already existing uncloak reasons.
		if (!TryUncloak(clientId, reason) && !clientIdsRequestingUncloak.contains(clientId))
			clientIdsRequestingUncloak[clientId] = reason;
	}

	static void AttemptInitialUncloak(uint clientId)
	{
		if (IsValidCloakableClient(clientId) && !clientCloakStats[clientId].initialUncloakCompleted && clientCloakStats[clientId].cloakState == CloakState::Cloaked)
			TryUncloak(clientId, UncloakReason::Initial);
	}

	static bool CheckDockCall(const uint ship, const uint dockTargetId, const uint dockPortIndex, const DOCK_HOST_RESPONSE response)
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

	static void CheckPlayerInNoCloakArea(const uint clientId, const uint clientSystemId, const uint clientShipId)
	{
		boolean insideNoCloakArea = false;
		uint noCloakAreaNNVoiceMessageId = 0;
		UncloakReason uncloakReason;
		Vector shipPosition;
		Matrix shipOrientation;
		pub::SpaceObj::GetLocation(clientShipId, shipPosition, shipOrientation);
		if (noCloakAreasPerSystem.contains(clientSystemId))
		{
			for (NoCloakArea& area : noCloakAreasPerSystem[clientSystemId])
			{
				if (HkDistance3D(area.position, shipPosition) <= area.radius)
				{
					insideNoCloakArea = true;
					noCloakAreaNNVoiceMessageId = area.NNVoiceMessageId;
					uncloakReason = UncloakReason::NoCloakArea;
					break;
				}
			}
		}
		clientCloakStats[clientId].insideNoCloakArea = insideNoCloakArea;
		clientCloakStats[clientId].currentNoCloakAreaNNVoiceMessageId = noCloakAreaNNVoiceMessageId;
		const CloakState cloakState = clientCloakStats[clientId].cloakState;
		if (insideNoCloakArea && (cloakState == CloakState::Cloaked || cloakState == CloakState::Cloaking))
			QueueUncloak(clientId, uncloakReason);
	}

	static void UpdateStateByEffectTimings(const uint clientId, const mstime currentTime)
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

	static void FireCloakActivator(const uint clientId)
	{
		if (IsValidCloakableClient(clientId) && clientCloakStats[clientId].activatorCargoId)
		{
			XFireWeaponInfo info;
			info.object = clientCloakStats[clientId].shipId;
			info.hpIds.push_back(static_cast<ushort>(clientCloakStats[clientId].activatorCargoId));
			info.target.x = 1;
			info.target.y = 0;
			info.target.z = 0;
			Server.FireWeapon(clientId, info);
			HookClient->Send_FLPACKET_COMMON_FIREWEAPON(clientId, info);
		}
	}

	static mstime lastSynchronizeTimeStamp = 0;
	static std::unordered_map<uint, CloakState> previousCloakStates;

	// This is executed to make sure players that spawn into a system with other cloaking players have their visibility synced.
	void UpdateCloakClients()
	{
		if (lastSynchronizeTimeStamp == 0)
			lastSynchronizeTimeStamp = timeInMS();

		const mstime now = timeInMS();

		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			const uint clientId = HkGetClientIdFromPD(playerData);
			if (!IsValidCloakableClient(clientId))
				continue;

			const CloakState& cloakState = clientCloakStats[clientId].cloakState;

			// Synchronize cloak state to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].cloakCargoId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);
			// Synchronize fallback instant cloak to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].instantCloakCargoId, cloakState == CloakState::Cloaked || cloakState == CloakState::Uncloaking);

			// Synchronize activator state to all players
			SendEquipmentActivationState(clientId, clientCloakStats[clientId].activatorCargoId, cloakState == CloakState::Cloaking || cloakState == CloakState::Cloaked);

			// Uncloak player in no-cloak-zones
			CheckPlayerInNoCloakArea(clientId, playerData->iSystemID, playerData->iShipID);

			// Cloak state update when effect time has passed
			// This also sets the initial uncloaked flag.
			UpdateStateByEffectTimings(clientId, now);

			// The rest in the update loop should be ignored for not initially uncloaked ships.
			if (!clientCloakStats[clientId].initialUncloakCompleted)
				continue;

			SynchronizeShieldStateWithCloakState(clientId);

			float power;
			clientCloakStats[clientId].shipInspect->get_power(power);

			// Uncloak when power is empty.
			if (cloakState == CloakState::Cloaked && power <= LOW_ENERGY_THRESHOLD)
			{
				QueueUncloak(clientId, UncloakReason::Power);
			}

			SynchronizePowerStateWithCloakState(clientId, power);

			// Schedule cloak changes.
			if (clientIdsRequestingUncloak.contains(clientId))
				QueueUncloak(clientId, clientIdsRequestingUncloak[clientId]);

			// Consume energy.
			if (cloakState == CloakState::Cloaked)
				FireCloakActivator(clientId);

			// Make sure to remove this player from everyone's mark list when the cloak state changed.
			if (previousCloakStates[clientId] != cloakState)
			{
				if (cloakState == CloakState::Cloaked)
					Mark::AddCloakedPlayer(clientId);
				else
					Mark::RemoveCloakedPlayer(clientId);
			}
			previousCloakStates[clientId] = cloakState;
		}
		lastSynchronizeTimeStamp = now;
		previousCloakStates.clear();
	}

	CloakState GetClientCloakState(uint clientId)
	{
		if (clientCloakStats.contains(clientId))
			return clientCloakStats[clientId].cloakState;
		return CloakState::Uncloaked;
	}

	static bool ToggleClientCloakActivator(const uint clientId, const bool active)
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
		if (IsValidCloakableClient(clientId))
		{
			if (clientCloakStats[clientId].activatorCargoId == activateEquip.sID)
				ToggleClientCloakActivator(clientId, activateEquip.bActivate);

			if (clientCloakStats[clientId].engineCargoId == activateEquip.sID)
				clientCloakStats[clientId].engineKillActive = !activateEquip.bActivate;
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId)
	{
		const uint clientId = HkGetClientIDByShip(shipId);
		if (!clientId)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		QueueUncloak(clientId, UncloakReason::User);

		if (IsValidCloakableClient(clientId))
			clientCloakStats[clientId].dockingManeuverActive = false;

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId)
	{
		ClearClientData(clientId);
		InstallCloak(clientId);

		returncode = DEFAULT_RETURNCODE;
	}
	
	int __cdecl Dock_Call(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response)
	{
		if (!CheckDockCall(ship, dockTargetId, dockPortIndex, response))
		{
			returncode = NOFUNCTIONCALL;
			return 0;
		}
		returncode = DEFAULT_RETURNCODE;
		return 0;
	}

	void __stdcall GoTradelane(unsigned int clientId, struct XGoTradelane const& goToTradelane)
	{
		QueueUncloak(clientId, UncloakReason::Docking);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
	{
		ClearClientData(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
	{
		RemoveCloakingDevices(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		if (!EquipCloakingDevices(clientId))
			RemoveCloakingDevices(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	static void SetServerSideEngineState(const uint clientId, const bool active)
	{
		if (IsValidCloakableClient(clientId) && clientCloakStats[clientId].engineCargoId)
		{
			XActivateEquip activateEquipment;
			activateEquipment.bActivate = active;
			activateEquipment.iSpaceID = clientCloakStats[clientId].shipId;
			activateEquipment.sID = clientCloakStats[clientId].engineCargoId;
			Server.ActivateEquip(clientId, activateEquipment);
			clientCloakStats[clientId].engineKillActive = !active;
		}
	}

	void __stdcall SPObjUpdate(SSPObjUpdateInfo const& updateInfo, unsigned int clientId)
	{
		AttemptInitialUncloak(clientId);

		// Fix bug of Throttle on server not being correctly set.
		if (IsValidCloakableClient(clientId))
		{
			// When saving the CShip result via CObject::Find permanently it always crashed the server
			// when a player with Cloak + Energy-using Engine docked to a base on zero energy.
			// Instead get it via the InspectionObj in the hope this is more performant than Find on each invokation.
			CShip* ship = (CShip*)(clientCloakStats[clientId].shipInspect->cobj);
			if (ship)
			{
				ship->set_throttle(updateInfo.throttle);

				// Setting Throttle on the ship makes the server think that engine was turned on again.
				if (clientCloakStats[clientId].engineKillActive)
					SetServerSideEngineState(clientId, false);
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state)
	{
		ClearClientData(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipEquipDestroyed(const IObjRW* object, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* damageList)
	{
		const uint clientId = object->cobj->GetOwnerPlayer();
		if (IsValidCloakableClient(clientId))
		{
			EquipDesc equipDescriptor;
			equip->GetEquipDesc(equipDescriptor);
			if (clientCloakStats[clientId].activatorCargoId == equipDescriptor.sID)
			{
				clientCloakStats[clientId].activatorCargoId = 0;
				QueueUncloak(clientId, UncloakReason::Destroyed);
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
	{
		auto& systemNoCloakAreas = noCloakAreasPerSystem[killedObject->cobj->system];
		for (auto it = systemNoCloakAreas.begin(); it != systemNoCloakAreas.end(); it++)
		{
			if (it->objectId == killedObject->cobj->get_id())
			{
				systemNoCloakAreas.erase(it);
				break;
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
	{
		if (killed)
		{
			const uint killedClientId = killedObject->cobj->GetOwnerPlayer();
			if (HkIsValidClientID(killedClientId))
				ClearClientData(killedClientId);
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void GuidedInit(CGuided* guided, CGuided::CreateParms& parms)
	{
		uint objId = parms.ownerId;
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem))
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		IObjRW* ownerTarget = nullptr;
		inspect->get_target(ownerTarget);
		if (!ownerTarget)
		{
			parms.target = nullptr;
			parms.subObjId = 0;
		}

		if (parms.target && (parms.target->cobj->objectClass & CObject::CEQOBJ_MASK) && static_cast<CEqObj*>(parms.target->cobj)->get_cloak_percentage() > 0.9f)
		{
			parms.target = nullptr;
			parms.subObjId = 0;
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void UserCmd_CLOAK(uint clientId, const std::wstring& wscParam)
	{
		if (IsValidCloakableClient(clientId))
		{
			const CloakState cloakState = clientCloakStats[clientId].cloakState;
			ToggleClientCloakActivator(clientId, cloakState == CloakState::Uncloaked || cloakState == CloakState::Uncloaking);
		}
	}

	void UserCmd_UNCLOAK(uint clientId, const std::wstring& wscParam)
	{
		if (IsValidCloakableClient(clientId))
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
		if (activateCruise.bActivate)
			SetServerSideEngineState(clientId, true);
		returncode = DEFAULT_RETURNCODE;
	}
}