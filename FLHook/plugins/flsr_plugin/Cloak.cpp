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
 * 
 * Specific Behavior:
 * 
 * * Shields will be kept disabled while being not fully uncloaked
 * * Works for NPCs and Solars, too. The following rules do NOT apply to them.
 * 
 * Cloaking devices will be automatically uncloaked when:
 * * Ship energy reaches zero
 * * flying into a No-Cloak-Area
 * * entering a Tradelane
 * * docking to any base or jump gate/jump hole
 * 
 * Cloaking is being blocked when:
 * * being in a running docking sequence
 * * jumping through a jump-tunnel until reaching the other side fully
 * * being in a Tradelane
 * 
 */

namespace Cloak
{
	float jumpGateDecloakRadius = 2000.0f;
	float jumpHoleDecloakRadius = 1000.0f;
	struct NoCloakArea
	{
		uint objectId = 0;
		Vector position;
		float radius = 0.0f;
		uint NNVoiceMessageId = 0;
	};
	std::unordered_map<uint, std::vector<NoCloakArea>> noCloakAreasPerSystem;
	std::unordered_map<uint, NoCloakArea> noCloakObjectDefinitionsByNicknameId;
	std::vector<NoCloakArea> unprocessedObjectNoCloakAreas;

	struct CloakForObjArchetypeDefinition
	{
		uint cloakingDeviceArchId = 0;
		uint cloakFuseId = 0;
		uint uncloakFuseId = 0;
		std::string cloakingDeviceHardpoint = "";
	};
	uint instantCloakingDeviceArchId = 0;
	std::unordered_set<uint> proxyArchetypeIds;
	std::unordered_map<uint, CloakForObjArchetypeDefinition> cloakForObjArchDefinitions;

	struct ObjectData
	{
		uint systemId = 0;
		CEquip* proxyEquip = nullptr;
		CECloakingDevice* mainCloak = nullptr;
		CECloakingDevice* instantCloak = nullptr;
		CEShield* shield = nullptr;
		// Player specific
		uint clientId = 0;
		std::unordered_set<CEPower*> powerGenerators{};
		bool dockingManeuverActive = false;
		mstime lastDrySoundTimeStamp = 0;
		mstime lastNeuralNetSoundTimeStamp = 0;
	};
	std::unordered_map<uint, ObjectData> objData;

	static void ReadFiles()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-Cloak.ini";

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
						else if (ini.is_value("instant_cloaking_device_nickname"))
							instantCloakingDeviceArchId = CreateID(ini.get_value_string(0));
					}
				}

				if (ini.is_header("Ship"))
				{
					uint archId = 0;
					CloakForObjArchetypeDefinition definition;
					while (ini.read_value())
					{
						if (ini.is_value("ship_nickname"))
							archId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("cloaking_device_nickname"))
							definition.cloakingDeviceArchId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("cloaking_device_hardpoint"))
							definition.cloakingDeviceHardpoint = ini.get_value_string(0);
						else if (ini.is_value("cloak_fuse_name"))
							definition.cloakFuseId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("uncloak_fuse_name"))
							definition.uncloakFuseId = CreateID(ini.get_value_string(0));
					}
					if (archId && definition.cloakingDeviceArchId && definition.cloakFuseId && definition.uncloakFuseId && !definition.cloakingDeviceHardpoint.empty())
						cloakForObjArchDefinitions.insert({ archId, definition });
				}

				if (ini.is_header("CloakActivators"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("activator_nickname"))
							proxyArchetypeIds.insert(CreateID(ini.get_value_string(0)));
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

	bool RegisterNoCloakSolar(const std::string& nickname, const uint objectId)
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
		const uint JUMP_GATE_NN_ID = CreateID("object_jump_gate");
		const uint JUMP_HOLE_NN_ID = CreateID("object_jump_hole");
		CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
		while (solar)
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

	bool initialized = false;
	// This must be executed AFTER ReadFiles and when the game data has already been stored to memory.
	void InitializeWithGameData()
	{
		if (initialized)
			return;
		initialized = true;

		ConPrint(L"Initializing Cloak... ");

		ReadFiles();
		CollectNoCloakObjectsPerSystem();

		ConPrint(L"Done\n");
	}

	bool RegisterObject(const uint objId)
	{
		if (objData.contains(objId))
			return false;

		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return false;

		const auto& objDefinition = cloakForObjArchDefinitions.find(inspect->cobj->archetype->iArchID);
		if (objDefinition == cloakForObjArchDefinitions.end())
			return false;

		ObjectData data;
		const auto& object = dynamic_cast<CEqObj*>(inspect->cobj);
		data.systemId = object->system;
		EquipDescVector equipList;
		object->get_equip_desc_list(equipList);
		for (const auto& entry : equipList.equip)
		{
			if (!entry.bMounted)
				continue;

			data.clientId = object->ownerPlayer;
			if (data.clientId)
			{
				const auto& equip = object->equip_manager.FindByID(entry.sID);
				if (equip && equip->CEquipType == EquipmentClass::Power)
				{
					data.powerGenerators.insert(dynamic_cast<CEPower*>(equip));
					continue;
				}
			}
			if (entry.iArchID == instantCloakingDeviceArchId)
			{
				const auto& equip = object->equip_manager.FindByID(entry.sID);
				if (equip && equip->CEquipType == EquipmentClass::CloakingDevice)
					data.instantCloak = dynamic_cast<CECloakingDevice*>(equip);
			}
			else if (entry.iArchID == objDefinition->second.cloakingDeviceArchId)
			{
				const auto& equip = object->equip_manager.FindByID(entry.sID);
				if (equip && equip->CEquipType == EquipmentClass::CloakingDevice)
					data.mainCloak = dynamic_cast<CECloakingDevice*>(equip);
			}
			else if (proxyArchetypeIds.contains(entry.iArchID))
				data.proxyEquip = object->equip_manager.FindByID(entry.sID);
		}
		data.shield = dynamic_cast<CEShield*>(object->equip_manager.FindFirst(EquipmentClass::Shield));

		if (data.instantCloak && data.mainCloak && data.proxyEquip && data.systemId)
			return objData.insert({ objId, data }).second;
		return false;
	}

	std::unordered_map<uint, bool> objIdsToTransition;

	void SetTargetCloakState(const uint objId, const bool cloaked)
	{
		const auto& objEntry = objData.find(objId);
		if (objEntry == objData.end() || !objEntry->second.proxyEquip)
			return;

		const float cloakPercentage = objEntry->second.mainCloak->cloak_percent();
		if (cloaked && cloakPercentage == 1.0f || !cloaked && cloakPercentage == 0.0f)
			return;

		const auto& result = objIdsToTransition.insert({ objId, cloaked });
		if (!result.second)
			result.first->second = cloaked;
	}

	bool IsFullyUncloaked(const uint objId)
	{
		if (const auto& objEntry = objData.find(objId); objEntry != objData.end())
			return objEntry->second.mainCloak->cloak_percent() == 0.0f;
		return true;
	}

	static void SynchronizeWeaponGroupsWithCloakState(const uint clientId, const bool cloakActive)
	{
		std::string proxyEquipHardpoint = "";
		const auto& playerData = Players[clientId];
		const EquipDescList& playerEquipList = Players[clientId].equipDescList;
		for (const auto& entry : playerEquipList.equip)
		{
			if (entry.bMounted && proxyArchetypeIds.contains(entry.iArchID))
			{
				proxyEquipHardpoint = entry.szHardPoint.value;
				break;
			}
		}
		if (proxyEquipHardpoint.empty())
			return;

		std::string currentWeaponGroups(playerData.weaponGroups.c_str());
		std::vector<std::string> lines;
		size_t delimiterPos;
		while ((delimiterPos = currentWeaponGroups.find('\n')) != std::string::npos)
		{
			std::string line = currentWeaponGroups.substr(0, delimiterPos);
			if (!line.empty() && line.find(proxyEquipHardpoint) == std::string::npos)
				lines.push_back(currentWeaponGroups.substr(0, delimiterPos));
			currentWeaponGroups.erase(0, delimiterPos + 1);
		}

		if (cloakActive)
		{
			for (int groupIndex = 0; groupIndex < 6; groupIndex++)
				lines.push_back("wg = " + std::to_string(groupIndex) + ", " + proxyEquipHardpoint);
		}

		std::string newWeaponGroups;
		for (const auto& line : lines)
			newWeaponGroups.append(line + "\n");

		Server.SetWeaponGroup(clientId, (unsigned char*)newWeaponGroups.c_str(), newWeaponGroups.size() + 1);
		GetClientInterface()->Send_FLPACKET_COMMON_SET_WEAPON_GROUP(clientId, (unsigned char*)newWeaponGroups.c_str(), newWeaponGroups.size() + 1);
		return;
	}

	static void SendEquipmentActivationState(const uint clientId, const uint shipId, const uint cargoId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.bActivate = active;
		activateEquipment.iSpaceID = shipId;
		activateEquipment.sID = cargoId;
		Server.ActivateEquip(clientId, activateEquipment);
		GetClientInterface()->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	static void BroadcastEquipmentActivationState(const uint systemId, const uint objId, const uint subObjId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.iSpaceID = objId;
		activateEquipment.sID = subObjId;
		activateEquipment.bActivate = active;
		const uint clientId = objData.at(objId).clientId;
		Server.ActivateEquip(clientId, activateEquipment);

		const auto& clientInterface = GetClientInterface();
		PlayerData* playerData = nullptr;
		while (playerData = Players.traverse_active(playerData))
		{
			if (!playerData->iBaseID && playerData->iSystemID == systemId)
				clientInterface->Send_FLPACKET_COMMON_ACTIVATEEQUIP(playerData->iOnlineID, activateEquipment);
		}
	}

	static void UpdateCloakStates()
	{
		for (auto transitionIt = objIdsToTransition.begin(); transitionIt != objIdsToTransition.end();)
		{
			const bool cloakedStateTargetted = transitionIt->second;
			const auto& objEntry = objData.find(transitionIt->first);
			if (objEntry == objData.end())
				continue;

			const auto& objData = objEntry->second;
			const float currentCloakPercentage = objEntry->second.mainCloak->cloak_percent();

			if (currentCloakPercentage == 1.0f)
			{
				objData.instantCloak->Activate(true);
				BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.instantCloak->iSubObjId, true);
			}
			else if (currentCloakPercentage == 0.0f)
			{
				objData.instantCloak->Activate(false);
				BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.instantCloak->iSubObjId, false);
			}

			// Full cloak desired and reached
			if (cloakedStateTargetted && currentCloakPercentage == 1.0f)
			{
				Mark::AddInvisibleObject(transitionIt->first);
				transitionIt = objIdsToTransition.erase(transitionIt);
				continue;
			}

			// Full uncloak desired and reached
			if (!cloakedStateTargetted && currentCloakPercentage == 0.0f)
			{
				transitionIt = objIdsToTransition.erase(transitionIt);
				if (objData.shield)
				{
					objData.shield->Activate(true);
					BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.shield->iSubObjId, true);
				}
				if (objData.clientId)
				{
					for (const auto& power : objData.powerGenerators)
						SendEquipmentActivationState(objData.clientId, objEntry->first, power->iSubObjId, true);
				}
				continue;
			}

			// Full cloak desired and full uncloak reached
			if (cloakedStateTargetted && currentCloakPercentage == 0.0f)
			{
				IObjRW* inspect;
				StarSystem* starSystem;
				if (GetShipInspect(objEntry->first, inspect, starSystem))
				{
					uint cloakFuseId = cloakForObjArchDefinitions.at(inspect->cobj->archetype->iArchID).cloakFuseId;
					inspect->unlight_fuse_unk(&cloakFuseId, 0, 0.0f);
					inspect->light_fuse(0, &cloakFuseId, 0, 0.0f, 0.0f);
				}
				objData.mainCloak->Activate(true);
				BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.mainCloak->iSubObjId, true);
				BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.proxyEquip->iSubObjId, true);
				if (objData.shield)
				{
					objData.shield->Activate(false);
					BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.shield->iSubObjId, false);
				}
				if (objData.clientId)
				{
					SynchronizeWeaponGroupsWithCloakState(objData.clientId, true);
					for (const auto& power : objData.powerGenerators)
						SendEquipmentActivationState(objData.clientId, objEntry->first, power->iSubObjId, false);
				}
			}
			// Full uncloak desired and full cloak reached
			else if (!cloakedStateTargetted && currentCloakPercentage == 1.0f)
			{
				IObjRW* inspect;
				StarSystem* starSystem;
				if (GetShipInspect(objEntry->first, inspect, starSystem))
				{
					uint uncloakFuseId = cloakForObjArchDefinitions.at(inspect->cobj->archetype->iArchID).uncloakFuseId;
					inspect->unlight_fuse_unk(&uncloakFuseId, 0, 0.0f);
					inspect->light_fuse(0, &uncloakFuseId, 0, 0.0f, 0.0f);
				}
				objData.mainCloak->Activate(false);
				BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.mainCloak->iSubObjId, false);
				BroadcastEquipmentActivationState(objData.systemId, objEntry->first, objData.proxyEquip->iSubObjId, false);
				Mark::RemoveInvisibleObject(objEntry->first);
				if (objData.clientId)
					SynchronizeWeaponGroupsWithCloakState(objData.clientId, false);
			}
			transitionIt++;
		}
	}

	const mstime NEURAL_NET_DEBOUNCE_TIME = 2000;
	const uint WARNING_NN_ID = CreateID("warning");
	static void PlayNoCloakAreaNeuralNetWarning(ObjectData& obj, const NoCloakArea& foundNoCloakArea)
	{
		const mstime now = timeInMS();
		if (now - obj.lastNeuralNetSoundTimeStamp > NEURAL_NET_DEBOUNCE_TIME && foundNoCloakArea.NNVoiceMessageId)
		{
			obj.lastNeuralNetSoundTimeStamp = now;
			pub::Player::SendNNMessage(obj.clientId, WARNING_NN_ID);
			pub::Player::SendNNMessage(obj.clientId, foundNoCloakArea.NNVoiceMessageId);
		}
	}

	static bool FindIntersectingNoCloakArea(const uint shipId, NoCloakArea& foundNoCloakArea)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(shipId, inspect, starSystem))
			return false;
		const auto& noCloakAreas = noCloakAreasPerSystem.find(inspect->cobj->system);
		if (noCloakAreas == noCloakAreasPerSystem.end())
			return false;

		for (const auto& area : noCloakAreas->second)
		{
			if (HkDistance3D(area.position, inspect->get_position()) <= area.radius)
			{
				foundNoCloakArea = area;
				return true;
			}
		}
		return false;
	}

	const float LOW_ENERGY_THRESHOLD = 100.0f;
	static void ConsumePower()
	{
		for (const auto& objEntry : objData)
		{
			if (!objEntry.second.clientId || !objEntry.second.mainCloak->IsActive())
				continue;

			IObjRW* inspect;
			StarSystem* starSystem;
			if (float power = 0.0f; GetShipInspect(objEntry.first, inspect, starSystem) && inspect->get_power(power) == 0 && power <= LOW_ENERGY_THRESHOLD)
				SetTargetCloakState(objEntry.first, false);
			else if (objEntry.second.proxyEquip)
			{
				XFireWeaponInfo info;
				info.object = objEntry.first;
				info.hpIds.push_back(objEntry.second.proxyEquip->iSubObjId);
				info.target.x = 1;
				info.target.y = 0;
				info.target.z = 0;
				Server.FireWeapon(objEntry.second.clientId, info);
				GetClientInterface()->Send_FLPACKET_COMMON_FIREWEAPON(objEntry.second.clientId, info);
			}
		}
	}

	float elapsedTimeInSec = 0.0f;
	void __stdcall Elapse_Time_AFTER(float seconds)
	{
		elapsedTimeInSec += seconds;
		if (elapsedTimeInSec < 0.1f)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}
		elapsedTimeInSec = 0.0f;

		ConsumePower();

		for (auto& objEntry : objData)
		{
			if (objEntry.second.clientId && objEntry.second.mainCloak->IsActive())
			{
				if (NoCloakArea noCloakArea; FindIntersectingNoCloakArea(objEntry.first, noCloakArea))
				{
					PlayNoCloakAreaNeuralNetWarning(objEntry.second, noCloakArea);
					SetTargetCloakState(objEntry.first, false);
				}
			}
		}

		UpdateCloakStates();

		returncode = DEFAULT_RETURNCODE;
	}

	void GuidedInit(CGuided* guided, CGuided::CreateParms& parms)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(parms.ownerId, inspect, starSystem))
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

		if (parms.target && (parms.target->cobj->objectClass & CObject::CEQOBJ_MASK) && dynamic_cast<CEqObj*>(parms.target->cobj)->get_cloak_percentage() > 0.9f)
		{
			parms.target = nullptr;
			parms.subObjId = 0;
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipAndSolarDestroyed(IObjRW* killedObj, bool killed, uint killerObjId)
	{
		const uint objId = killedObj->get_id();
		objData.erase(objId);
		objIdsToTransition.erase(objId);
		if (const auto& systemEntry = noCloakAreasPerSystem.find(killedObj->cobj->system); systemEntry != noCloakAreasPerSystem.end())
		{
			for (auto it = systemEntry->second.begin(); it != systemEntry->second.end(); it++)
			{
				if (it->objectId == killedObj->cobj->get_id())
				{
					systemEntry->second.erase(it);
					break;
				}
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipAndSolarEquipDestroyed(IObjRW* obj, CEquip* equip, DamageEntry::SubObjFate fate, DamageList* dmgList)
	{
		const uint objId = obj->get_id();
		const auto& objEntry = objData.find(objId);
		if (objEntry != objData.end())
		{
			if (objEntry->second.proxyEquip == equip)
			{
				SetTargetCloakState(objId, false);
				objEntry->second.proxyEquip = nullptr;
			}
			else if (objEntry->second.instantCloak == equip || objEntry->second.mainCloak == equip)
			{
				objData.erase(objId);
				objIdsToTransition.erase(objId);
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall RequestCreateShip_After(unsigned int clientId)
	{
		// Initially remove all cloak activators from the weapons groups. Otherwise they may send an ActivateEquip event to the server.
		if (cloakForObjArchDefinitions.contains(Players[clientId].iShipArchetype))
			SynchronizeWeaponGroupsWithCloakState(clientId, false);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
	{
		// Players always must start with their cloak being deactivated.
		if (RegisterObject(shipId))
		{
			if (const auto& objEntry = objData.find(shipId); objEntry != objData.end())
			{
				objEntry->second.mainCloak->Activate(false);
				objEntry->second.instantCloak->Activate(false);
				SendEquipmentActivationState(clientId, shipId, objEntry->second.mainCloak->iSubObjId, objEntry->second.mainCloak->IsActive());
				SendEquipmentActivationState(clientId, shipId, objEntry->second.instantCloak->iSubObjId, objEntry->second.instantCloak->IsActive());
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	static void SendCloakingDeviceState(const uint clientId, const uint objId)
	{
		if (const auto& objEntry = objData.find(objId); objEntry != objData.end())
		{
			SendEquipmentActivationState(clientId, objId, objEntry->second.mainCloak->iSubObjId, objEntry->second.mainCloak->IsActive());
			SendEquipmentActivationState(clientId, objId, objEntry->second.instantCloak->iSubObjId, objEntry->second.instantCloak->IsActive());
		}
	}

	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet)
	{
		SendCloakingDeviceState(clientId, packet.iSpaceID);
		returncode = DEFAULT_RETURNCODE;
		return true;
	}

	bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet)
	{
		SendCloakingDeviceState(clientId, packet.iSpaceID);
		returncode = DEFAULT_RETURNCODE;
		return true;
	}

	int __cdecl Dock_Call(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response)
	{
		if (auto objEntry = objData.find(shipId); objEntry != objData.end() && objEntry->second.clientId)
		{
			objEntry->second.dockingManeuverActive = dockTargetId && dockPortIndex >= 0 && (response == DOCK_HOST_RESPONSE::PROCEED_DOCK || response == DOCK_HOST_RESPONSE::DOCK);
			if (objEntry->second.dockingManeuverActive)
				SetTargetCloakState(shipId, false);
		}
		returncode = DEFAULT_RETURNCODE;
		return 0;
	}

	void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId)
	{
		if (auto objEntry = objData.find(shipId); objEntry != objData.end() && objEntry->second.clientId)
		{
			SetTargetCloakState(shipId, false);
			objEntry->second.dockingManeuverActive = false;
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall GoTradelane(unsigned int clientId, struct XGoTradelane const& goToTradelane)
	{
		uint shipId = 0;
		pub::Player::GetShip(clientId, shipId);
		if (shipId)
			SetTargetCloakState(shipId, false);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
	{
		const EquipDescList& playerEquipList = Players[clientId].equipDescList;
		for (const auto& entry : playerEquipList.equip)
		{
			const Archetype::Equipment* equip = Archetype::GetEquipment(entry.iArchID);
			if (equip && equip->get_class_type() == Archetype::AClassType::CLOAKING_DEVICE)
				pub::Player::RemoveCargo(clientId, entry.sID, entry.iCount);
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		const auto& objDefinition = cloakForObjArchDefinitions.find(Players[clientId].iShipArchetype);
		if (objDefinition != cloakForObjArchDefinitions.end())
		{
			const EquipDescList& playerEquipList = Players[clientId].equipDescList;
			for (const auto& entry : playerEquipList.equip)
			{
				if (entry.bMounted && proxyArchetypeIds.contains(entry.iArchID))
				{
					HkAddEquip(ARG_CLIENTID(clientId), instantCloakingDeviceArchId, objDefinition->second.cloakingDeviceHardpoint, 1, true);
					HkAddEquip(ARG_CLIENTID(clientId), objDefinition->second.cloakingDeviceArchId, objDefinition->second.cloakingDeviceHardpoint, 1, true);
					break;
				}
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	const mstime DRY_SOUND_DEBOUNCE_TIME = 1000;
	const uint FIRE_DRY_ID = CreateID("fire_dry");
	static void PlayDrySound(ObjectData& obj)
	{
		const mstime now = timeInMS();
		if (now - obj.lastDrySoundTimeStamp > DRY_SOUND_DEBOUNCE_TIME)
		{
			obj.lastDrySoundTimeStamp = now;
			pub::Audio::PlaySoundEffect(obj.clientId, FIRE_DRY_ID);
		}
	}

	static bool TrySetCloakState(const uint clientId, const bool activate)
	{
		uint shipId = 0;
		pub::Player::GetShip(clientId, shipId);
		auto objEntry = objData.find(shipId);
		if (objEntry == objData.end())
			return false;

		if (objIdsToTransition.contains(shipId))
		{
			PlayDrySound(objEntry->second);
			return false;
		}

		if (!activate)
		{
			SetTargetCloakState(shipId, activate);
			return false;
		}

		if (NoCloakArea noCloakArea; FindIntersectingNoCloakArea(shipId, noCloakArea))
		{
			PlayDrySound(objEntry->second);
			PlayNoCloakAreaNeuralNetWarning(objEntry->second, noCloakArea);
			return false;
		}

		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(shipId, inspect, starSystem))
			return false;
		bool inTradelane = false;
		if (objEntry->second.dockingManeuverActive || (inspect->is_using_tradelane(&inTradelane) == 0 && inTradelane))
		{
			PlayDrySound(objEntry->second);
			return false;
		}

		SetTargetCloakState(shipId, activate);
		return true;
	}

	void __stdcall ActivateEquip(unsigned int clientId, XActivateEquip const& activateEquip)
	{
		if (const auto objEntry = objData.find(activateEquip.iSpaceID); objEntry != objData.end() && objEntry->second.clientId == clientId && objEntry->second.proxyEquip && objEntry->second.proxyEquip->iSubObjId == activateEquip.sID)
		{
			if (!TrySetCloakState(clientId, activateEquip.bActivate))
			{
				SendEquipmentActivationState(clientId, activateEquip.iSpaceID, activateEquip.sID, objEntry->second.mainCloak->IsActive());
				returncode = NOFUNCTIONCALL;
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void UserCmd_CLOAK(uint clientId, const std::wstring& wscParam)
	{
		uint shipId = 0;
		pub::Player::GetShip(clientId, shipId);
		if (const auto objEntry = objData.find(shipId); objEntry != objData.end())
			TrySetCloakState(clientId, objEntry->second.mainCloak->cloak_percent() == 0.0f);
	}

	void UserCmd_UNCLOAK(uint clientId, const std::wstring& wscParam)
	{
		uint shipId = 0;
		pub::Player::GetShip(clientId, shipId);
		if (const auto objEntry = objData.find(shipId); objEntry != objData.end())
			TrySetCloakState(clientId, false);
	}
}
