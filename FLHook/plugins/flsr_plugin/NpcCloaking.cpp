#include "NpcCloaking.h"
#include "plugin.h"

namespace NpcCloaking
{
	const ushort SHIELD_SLOT_ID = 65521;

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
		uint systemId;
		CEquip* proxyEquip = nullptr;
		CECloakingDevice* mainCloak = nullptr;
		CECloakingDevice* instantCloak = nullptr;
		CEShield* shield = nullptr;
	};

	void ReadFiles()
	{
		ConPrint(L"Initializing NPC Cloaking... ");

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
						if (ini.is_value("instant_cloaking_device_nickname"))
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
			}
			ini.close();
		}

		ConPrint(L"Done\n");
	}

	std::unordered_map<uint, ObjectData> objData;

	void RegisterObject(uint objId)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return;

		const auto& objDefinition = cloakForObjArchDefinitions.find(inspect->cobj->archetype->iArchID);
		if (objDefinition == cloakForObjArchDefinitions.end())
			return;

		ObjectData data;
		const auto& object = dynamic_cast<CEqObj*>(inspect->cobj);
		data.systemId = object->system;
		EquipDescVector equipList;
		object->get_equip_desc_list(equipList);
		for (const auto& entry : equipList.equip)
		{
			if (!entry.bMounted)
				continue;

			if (entry.iArchID == instantCloakingDeviceArchId)
				data.instantCloak = dynamic_cast<CECloakingDevice*>(object->equip_manager.FindByID(entry.sID));
			else if (entry.iArchID == objDefinition->second.cloakingDeviceArchId)
				data.mainCloak = dynamic_cast<CECloakingDevice*>(object->equip_manager.FindByID(entry.sID));
			else if (proxyArchetypeIds.contains(entry.iArchID))
				data.proxyEquip = (object->equip_manager.FindByID(entry.sID));
		}
		data.shield = dynamic_cast<CEShield*>(object->equip_manager.FindByID(SHIELD_SLOT_ID));

		if (data.instantCloak && data.mainCloak && data.proxyEquip && data.systemId)
			objData.insert({ objId, data });
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


	static void BroadcastEquipmentActivationState(const uint systemId, const uint objId, const uint subObjId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.iSpaceID = objId;
		activateEquipment.sID = subObjId;
		activateEquipment.bActivate = active;

		const auto& clientInterface = GetClientInterface();
		PlayerData* playerData = nullptr;
		while (playerData = Players.traverse_active(playerData))
		{
			if (!playerData->iBaseID && playerData->iSystemID == systemId)
				clientInterface->Send_FLPACKET_COMMON_ACTIVATEEQUIP(playerData->iOnlineID, activateEquipment);
		}
	}

	static void Update()
	{
		for (auto transitionIt = objIdsToTransition.begin(); transitionIt != objIdsToTransition.end();)
		{
			const bool cloakedStateTargetted = transitionIt->second;
			const auto& objEntry = objData.find(transitionIt->first);
			if (objEntry == objData.end())
				continue;

			const float currentCloakPercentage = objEntry->second.mainCloak->cloak_percent();

			if (currentCloakPercentage == 1.0f)
			{
				objEntry->second.instantCloak->Activate(true);
				BroadcastEquipmentActivationState(objEntry->second.systemId, objEntry->first, objEntry->second.instantCloak->GetID(), true);
			}
			else if (currentCloakPercentage == 0.0f)
			{
				objEntry->second.instantCloak->Activate(false);
				BroadcastEquipmentActivationState(objEntry->second.systemId, objEntry->first, objEntry->second.instantCloak->GetID(), false);
			}

			// Full cloak desired and reached
			if (cloakedStateTargetted && currentCloakPercentage == 1.0f)
			{
				transitionIt = objIdsToTransition.erase(transitionIt);
				continue;
			}

			// Full uncloak desired and reached
			if (!cloakedStateTargetted && currentCloakPercentage == 0.0f)
			{
				transitionIt = objIdsToTransition.erase(transitionIt);
				if (objEntry->second.shield)
				{
					objEntry->second.shield->Activate(true);
					BroadcastEquipmentActivationState(objEntry->second.systemId, objEntry->first, SHIELD_SLOT_ID, true);
				}
				continue;
			}

			// Full cloak desired but just reached full uncloak
			if (cloakedStateTargetted && currentCloakPercentage == 0.0f)
			{
				uint objId = objEntry->first;
				IObjRW* inspect;
				StarSystem* starSystem;
				if (GetShipInspect(objId, inspect, starSystem))
				{
					uint cloakFuseId = cloakForObjArchDefinitions.at(inspect->cobj->archetype->iArchID).cloakFuseId;
					inspect->unlight_fuse_unk(&cloakFuseId, 0, 0.0f);
					inspect->light_fuse(0, &cloakFuseId, 0, 0.0f, 0.0f);
				}
				objEntry->second.mainCloak->Activate(true);
				BroadcastEquipmentActivationState(objEntry->second.systemId, objEntry->first, objEntry->second.mainCloak->GetID(), true);
				if (objEntry->second.shield)
				{
					objEntry->second.shield->Activate(false);
					BroadcastEquipmentActivationState(objEntry->second.systemId, objEntry->first, SHIELD_SLOT_ID, false);
				}
			}
			// Full uncloak desired but just reached full cloak
			else if (!cloakedStateTargetted && currentCloakPercentage == 1.0f)
			{
				uint objId = objEntry->first;
				IObjRW* inspect;
				StarSystem* starSystem;
				if (GetShipInspect(objId, inspect, starSystem))
				{
					uint uncloakFuseId = cloakForObjArchDefinitions.at(inspect->cobj->archetype->iArchID).uncloakFuseId;
					inspect->unlight_fuse_unk(&uncloakFuseId, 0, 0.0f);
					inspect->light_fuse(0, &uncloakFuseId, 0, 0.0f, 0.0f);
				}
				objEntry->second.mainCloak->Activate(false);
				BroadcastEquipmentActivationState(objEntry->second.systemId, objEntry->first, objEntry->second.mainCloak->GetID(), false);
			}
			transitionIt++;
		}
	}

	float elapsedTimeInSec = 0.0f;
	void __stdcall Elapse_Time_AFTER(float seconds)
	{
		elapsedTimeInSec += seconds;
		if (elapsedTimeInSec < 0.2f)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}
		elapsedTimeInSec = 0.0f;

		Update();
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipAndSolarDestroyed(IObjRW* killedObj, bool killed, uint killerObjId)
	{
		const uint objId = killedObj->get_id();
		objData.erase(objId);
		objIdsToTransition.erase(objId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ShipAndSolarEquipDestroyedHook(IObjRW* obj, CEquip* equip, DamageEntry::SubObjFate fate, DamageList* dmgList)
	{
		const auto& objEntry = objData.find(obj->get_id());
		if (objEntry != objData.end() && objEntry->second.proxyEquip == equip)
		{
			SetTargetCloakState(obj->get_id(), false);
			objEntry->second.proxyEquip = nullptr;
		}
		returncode = DEFAULT_RETURNCODE;
	}

	static void SendEquipmentActivationState(const uint clientId, const uint objId, const uint subObjId, const bool active)
	{
		XActivateEquip activateEquipment;
		activateEquipment.iSpaceID = objId;
		activateEquipment.sID = subObjId;
		activateEquipment.bActivate = active;
		GetClientInterface()->Send_FLPACKET_COMMON_ACTIVATEEQUIP(clientId, activateEquipment);
	}

	static void SendCloakingDeviceState(const uint clientId, uint objId, const EquipDescVector& equipList)
	{
		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return;
		const auto& object = dynamic_cast<CEqObj*>(inspect->cobj);

		for (const auto& entry : equipList.equip)
		{
			if (!entry.bMounted)
				continue;

			const auto& equip = object->equip_manager.FindByID(entry.sID);
			if (equip->CEquipType == EquipmentClass::CloakingDevice && !equip->IsActive())
				SendEquipmentActivationState(clientId, objId, entry.sID, equip->IsActive());
		}
	}

	static void SendShieldState(const uint clientId, uint objId)
	{
		float currentShieldCapacity;
		float maxShieldCapacity;
		bool shieldsUp;
		pub::SpaceObj::GetShieldHealth(objId, currentShieldCapacity, maxShieldCapacity, shieldsUp);
		if (maxShieldCapacity <= 0.0f)
			return;

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return;
		const auto& object = dynamic_cast<CEqObj*>(inspect->cobj);
		const auto& equip = object->equip_manager.FindByID(SHIELD_SLOT_ID);
		if (equip && !equip->IsActive())
			SendEquipmentActivationState(clientId, objId, SHIELD_SLOT_ID, equip->IsActive());
	}

	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet)
	{
		if (!packet.clientId)
		{
			SendCloakingDeviceState(clientId, packet.iSpaceID, packet.equip);
			SendShieldState(clientId, packet.iSpaceID);
		}
		returncode = DEFAULT_RETURNCODE;
		return true;
	}

	bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet)
	{
		SendCloakingDeviceState(clientId, packet.iSpaceID, packet.equip);
		SendShieldState(clientId, packet.iSpaceID);
		returncode = DEFAULT_RETURNCODE;
		return true;
	}
}