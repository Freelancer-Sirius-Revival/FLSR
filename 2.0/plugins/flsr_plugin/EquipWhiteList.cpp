#include "Main.h"

namespace EquipWhiteList
{
	const uint TIMER_INTERVAL = 50;
	const mstime UNMOUNT_CHECK_DELAY = 200;

	const uint NO_PLACE_TO_MOUNT_ID = pub::GetNicknameId("no_place_to_mount");
	const uint LOADED_INTO_CARGO_HOLD_ID = pub::GetNicknameId("loaded_into_cargo_hold");

	static std::map<uint, std::vector<uint>> allowedEquipmentArchetypeIdsPerShipArchetypeId;
	static std::map<uint, mstime> delayedClientIdEquipmentChecks;

	void LoadEquipWhiteList()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		std::string configFilePath = std::string(currentDirectory) + Globals::Equip_WHITELIST_FILE;

		allowedEquipmentArchetypeIdsPerShipArchetypeId.clear();
		delayedClientIdEquipmentChecks.clear();
		INI_Reader ini;
		if (ini.open(configFilePath.c_str(), false))
		{
			while (ini.read_header())
			{
				const uint equipmentArchetypeId = CreateID(ini.get_header_ptr());
				while (ini.read_value())
				{
					if (ini.is_value("ship"))
						allowedEquipmentArchetypeIdsPerShipArchetypeId[equipmentArchetypeId].push_back(CreateID(ini.get_value_string(0)));
				}
			}
			ini.close();
		}
	}

	bool UnmountNotAllowedEquipment(uint clientId, const EquipDescList& equipmentList)
	{
		delayedClientIdEquipmentChecks.erase(clientId);
		if (!HkIsValidClientID(clientId))
			return false;

		std::vector<EquipDesc> illegalEquipments;
		for (const auto& equip : equipmentList.equip)
		{
			if (equip.bMounted && allowedEquipmentArchetypeIdsPerShipArchetypeId.contains(equip.iArchID))
			{
				bool foundMatchingShipArchetype = false;
				uint shipArchetypeId;
				pub::Player::GetShipID(clientId, shipArchetypeId);
				for (const auto& allowedShipArchetypeId : allowedEquipmentArchetypeIdsPerShipArchetypeId[equip.iArchID])
				{
					if (shipArchetypeId == allowedShipArchetypeId)
					{
						foundMatchingShipArchetype = true;
						break;
					}
				}
				if (!foundMatchingShipArchetype)
				{
					illegalEquipments.push_back(equip);
				}
			}
		}

		if (!illegalEquipments.empty())
		{
			pub::Player::SendNNMessage(clientId, NO_PLACE_TO_MOUNT_ID);
			pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);

			for (const auto& equip : illegalEquipments)
			{
				pub::Player::RemoveCargo(clientId, equip.sID, equip.iCount);
				pub::Player::AddCargo(clientId, equip.iArchID, equip.iCount, equip.get_status(), false);
			}
			return true;
		}
		return false;
	}

	// Safe-guard to make sure when the Whitelist is modified, players will get the changes when docking.
	void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("EquipWhiteListModule") && HkIsValidClientID(clientId))
		{
			if (UnmountNotAllowedEquipment(clientId, Players[clientId].equipDescList))
				PrintUserCmdText(clientId, L"Some equipment has been ummounted due to changed requirements.");
		}
	}

	// Safe-guard to make sure when the Whitelist is modified after players changed equipment the last time.
	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("CloakModule") && HkIsValidClientID(clientId))
		{
			if (UnmountNotAllowedEquipment(clientId, Players[clientId].equipDescList))
				PrintUserCmdText(clientId, L"Some equipment has been ummounted due to changed requirements.");
		}
	}

	// Called when equipment is being mounted/unmounted or it is transferred to a new ship.
	// Always happens BEFORE ReqShipArch_AFTER. To still use the new ship archetype, delay the actual checks.
	void __stdcall ReqEquipment_AFTER(class EquipDescList const& equipDescriptorList, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("EquipWhiteListModule") && HkIsValidClientID(clientId))
		{
			delayedClientIdEquipmentChecks[clientId] = timeInMS();
		}
	}

	// Called when equipment is being added to the ship from a trader.
	void __stdcall ReqAddItem_AFTER(unsigned int goodArchetypeId, char const* hardpoint, int count, float status, bool mounted, uint clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("EquipWhiteListModule"))
		{
			if (mounted && allowedEquipmentArchetypeIdsPerShipArchetypeId.contains(goodArchetypeId))
			{
				uint shipArchetypeId;
				pub::Player::GetShipID(clientId, shipArchetypeId);
				for (const auto& allowedShipArchetypeId : allowedEquipmentArchetypeIdsPerShipArchetypeId[goodArchetypeId])
				{
					if (shipArchetypeId == allowedShipArchetypeId)
						return;
				}

				pub::Player::SendNNMessage(clientId, NO_PLACE_TO_MOUNT_ID);
				pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);

				const auto hardpointString = std::string(hardpoint);
				for (const auto& equip : Players[clientId].equipDescList.equip)
				{
					if ((equip.iArchID == goodArchetypeId) && (std::string(equip.szHardPoint.value) == hardpointString))
					{
						pub::Player::RemoveCargo(clientId, equip.sID, equip.iCount);
						pub::Player::AddCargo(clientId, goodArchetypeId, equip.iCount, status, false);
						return;
					}
				}
			}
		}
	}

	// Safe-guard. Called when ship was purchased. This always happens AFTER ReqEquipment and is the reason for the slight time delay before checks truly happen.
	void __stdcall ReqShipArch_AFTER(unsigned int archetypeId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (Modules::GetModuleState("EquipWhiteListModule") && HkIsValidClientID(clientId))
		{
			delayedClientIdEquipmentChecks[clientId] = timeInMS();
		}
	}

	// Process any delayed equipment checks.
	void ProcessChangedEquipments()
	{
		if (Modules::GetModuleState("EquipWhiteListModule"))
		{
			const mstime now = timeInMS() - UNMOUNT_CHECK_DELAY;
			std::vector<uint> clientIdsToUnmountFrom;
			for (const auto& timeStampClientId : delayedClientIdEquipmentChecks)
			{
				if ((now >= timeStampClientId.second) && HkIsValidClientID(timeStampClientId.first))
					clientIdsToUnmountFrom.push_back(timeStampClientId.first);
			}

			for (const uint clientIdToUnmountFrom : clientIdsToUnmountFrom)
				UnmountNotAllowedEquipment(clientIdToUnmountFrom, Players[clientIdToUnmountFrom].equipDescList);
		}
	}
}