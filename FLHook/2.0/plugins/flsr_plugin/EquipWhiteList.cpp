#include "Main.h"

namespace EquipWhiteList
{
	const uint NO_PLACE_TO_MOUNT_ID = CreateID("no_place_to_mount");
	const uint LOADED_INTO_CARGO_HOLD_ID = CreateID("loaded_into_cargo_hold");

	static std::unordered_map<uint, std::unordered_set<uint>> allowedEquipmentArchetypeIdsPerShipArchetypeId;

	void LoadEquipWhiteList()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		std::string configFilePath = std::string(currentDirectory) + Globals::Equip_WHITELIST_FILE;

		allowedEquipmentArchetypeIdsPerShipArchetypeId.clear();
		INI_Reader ini;
		if (ini.open(configFilePath.c_str(), false))
		{
			while (ini.read_header())
			{
				const uint equipmentArchetypeId = CreateID(ini.get_header_ptr());
				while (ini.read_value())
				{
					if (ini.is_value("ship"))
						allowedEquipmentArchetypeIdsPerShipArchetypeId[equipmentArchetypeId].insert(CreateID(ini.get_value_string(0)));
				}
			}
			ini.close();
		}
	}

	static void SendNNMessages(const uint clientId)
	{
		pub::Player::SendNNMessage(clientId, NO_PLACE_TO_MOUNT_ID);
		pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);
	}

	static bool UnmountIllegalEquipment(EquipDescList& equipmentList, const uint shiparchId, const uint clientId)
	{
		bool equipWasDismounted = false;
		for (auto& equip : equipmentList.equip)
		{
			if (equip.bMounted
				&& allowedEquipmentArchetypeIdsPerShipArchetypeId.contains(equip.iArchID)
				&& !allowedEquipmentArchetypeIdsPerShipArchetypeId[equip.iArchID].contains(shiparchId)
				)
			{
				equip.set_equipped(false);
				equip.set_hardpoint(EquipDesc::CARGO_BAY_HP_NAME);
				equipWasDismounted = true;
			}
		}
		if (equipWasDismounted)
			SendNNMessages(clientId);
		return equipWasDismounted;
	}

	void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
	{
		uint shipArchetypeId;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		if (UnmountIllegalEquipment(Players[clientId].equipDescList, shipArchetypeId, clientId))
			PrintUserCmdText(clientId, L"Some equipment has been unmounted due to changed requirements.");
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		uint shipArchetypeId;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		if (UnmountIllegalEquipment(Players[clientId].equipDescList, shipArchetypeId, clientId))
			PrintUserCmdText(clientId, L"Some equipment has been unmounted due to changed requirements.");
		returncode = DEFAULT_RETURNCODE;
	}

	static std::unordered_map<uint, uint> boughtShipArchByClientId;

	void __stdcall GFGoodBuy(const SGFGoodBuyInfo& gbi, unsigned int clientId)
	{
		boughtShipArchByClientId.erase(clientId);
		const GoodInfo* goodInfo = GoodList::find_by_id(gbi.iGoodID);
		if (!goodInfo || goodInfo->iType != 3)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}
		const GoodInfo* hullGoodInfo = GoodList::find_by_id(goodInfo->iHullGoodID);
		if (hullGoodInfo && hullGoodInfo->iType == 2)
			boughtShipArchByClientId[clientId] = hullGoodInfo->iShipGoodID;
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ReqEquipment(const EquipDescList& equipDescriptorList, unsigned int clientId)
	{
		uint shipArchetypeId;
		if (boughtShipArchByClientId.contains(clientId))
		{
			shipArchetypeId = boughtShipArchByClientId[clientId];
			boughtShipArchByClientId.erase(clientId);
		}
		else
			pub::Player::GetShipID(clientId, shipArchetypeId);
		UnmountIllegalEquipment(*((EquipDescList*)&equipDescriptorList), shipArchetypeId, clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall ReqAddItem(unsigned int& goodArchetypeId, char* hardpoint, int& count, float& status, bool& mounted, uint clientId)
	{
		uint shipArchetypeId;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		if (allowedEquipmentArchetypeIdsPerShipArchetypeId.contains(goodArchetypeId)
			&& !allowedEquipmentArchetypeIdsPerShipArchetypeId[goodArchetypeId].contains(shipArchetypeId)
		)
		{
			mounted = false;
			strcpy(hardpoint, EquipDesc::CARGO_BAY_HP_NAME.value);
			SendNNMessages(clientId);
		}
		returncode = DEFAULT_RETURNCODE;
	}
}