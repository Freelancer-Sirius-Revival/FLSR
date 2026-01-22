#include "Main.h"
#include <random>

namespace LootBoxes
{
	std::mt19937 randomizer(std::random_device{}());

	struct LootBox
	{
		uint boxArchetypeId = 0;
		uint keyArchetypeId = 0;
		float boxArchetypeVolume = 0.0f;
		float keyArchetypeVolume = 0.0f;
		std::wstring boxArchetypeName = L"";
		std::wstring keyArchetypeName = L"";
		std::discrete_distribution<int> lootArchetypeIdsDistribution;
		std::vector<uint> lootArchetypeIds;
		std::vector<std::wstring> lootArchetypeNames;
		float highestLootArchetypeVolume = 0.0f;
	};

	std::unordered_map<std::string, LootBox> lootBoxes;
	std::unordered_map<uint, bool> lootArchetypeCombinable;

	uint successSoundId = 0;
	uint failSoundId = 0;

	const uint LOADED_INTO_CARGO_HOLD_ID = pub::GetNicknameId("loaded_into_cargo_hold");

	static float GetEquipmentVolume(const uint archetypeId)
	{
		const Archetype::Equipment* equipment = Archetype::GetEquipment(archetypeId);
		if (equipment)
			return equipment->fVolume;
		return 0.0f;
	}

	static std::wstring GetEquipmentName(const uint archetypeId)
	{
		const GoodInfo* goodInfo = GoodList::find_by_id(archetypeId);
		if (goodInfo)
		{
			std::wstring name = HkGetWStringFromIDS(goodInfo->iIDSName);
			std::replace<std::wstring::iterator, wchar_t>(name.begin(), name.end(), 0x2019 /* ’ */, '\'');
			return name;
		}
		return L"";
	}

	bool initialized = false;
	void ReadInitialData()
	{
		if (initialized)
			return;
		initialized = true;

		ConPrint(L"Initializing Loot Boxes... ");

		HkLoadStringDLLs();

		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + Globals::LOOTBOXES_CONFIG_FILE;

		INI_Reader ini;
		if (ini.open(configFilePath.c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("General"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("success_sound_nickname"))
						{
							successSoundId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("fail_sound_nickname"))
						{
							failSoundId = CreateID(ini.get_value_string(0));
						}
					}
				}
				if (ini.is_header("LootBox"))
				{
					LootBox lootBox;
					std::vector<int> probabilities;
					while (ini.read_value())
					{
						if (ini.is_value("box_nickname"))
						{
							lootBox.boxArchetypeId = CreateID(ini.get_value_string(0));
							lootBox.boxArchetypeVolume = GetEquipmentVolume(lootBox.boxArchetypeId);
							lootBox.boxArchetypeName = GetEquipmentName(lootBox.boxArchetypeId);
						}
						else if (ini.is_value("key_nickname"))
						{
							lootBox.keyArchetypeId = CreateID(ini.get_value_string(0));
							lootBox.keyArchetypeVolume = GetEquipmentVolume(lootBox.keyArchetypeId);
							lootBox.keyArchetypeName = GetEquipmentName(lootBox.keyArchetypeId);
						}
						else if (ini.is_value("loot_item"))
						{
							const uint archetypeId = CreateID(ini.get_value_string(0));
							const GoodInfo* goodInfo = GoodList::find_by_id(archetypeId);
							if (goodInfo)
								lootArchetypeCombinable[archetypeId] = goodInfo->multiCount;
							lootBox.lootArchetypeIds.push_back(archetypeId);
							lootBox.lootArchetypeNames.push_back(GetEquipmentName(archetypeId));
							lootBox.highestLootArchetypeVolume = std::max(lootBox.highestLootArchetypeVolume, GetEquipmentVolume(archetypeId));
							probabilities.push_back(ini.get_value_int(1));
						}
					}
					if (!lootBox.boxArchetypeName.empty() && lootBox.boxArchetypeId && !lootBox.lootArchetypeIds.empty())
					{
						lootBox.lootArchetypeIdsDistribution = std::discrete_distribution<int>({ probabilities.begin(), probabilities.end() });
						lootBoxes[ToLower(wstos(lootBox.boxArchetypeName))] = lootBox;
					}
				}
			}
			ini.close();

			ConPrint(L"Done\n");
		}
	}

	static std::vector<CARGO_INFO> GetUnmountedCargoList(const uint clientId)
	{
		int remainingHoldSize;
		std::list<CARGO_INFO> cargoList;
		if (HkEnumCargo(ARG_CLIENTID(clientId), cargoList, remainingHoldSize) != HKE_OK)
			return std::vector<CARGO_INFO>();

		std::vector<CARGO_INFO> filteredCargoList;
		for (const auto& entry : cargoList)
		{
			if (!entry.bMounted)
				filteredCargoList.push_back(entry);
		}

		return filteredCargoList;
	}

	static bool Open(const uint clientId, const std::string lootBoxName, int openCount)
	{
		if (openCount < 1)
			openCount = 1;

		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		// Check if the loot box exists.
		if (lootBoxName.empty())
		{
			PrintUserCmdText(clientId, L"Item name must be given to open it!");
			return false;
		}

		const std::string lootBoxNameLower = ToLower(lootBoxName);
		if (!lootBoxes.contains(lootBoxNameLower))
		{
			PrintUserCmdText(clientId, L"'" + stows(lootBoxName) + L"' cannot be opened!");
			return false;
		}
		const LootBox& lootBox = lootBoxes[lootBoxNameLower];

		// Check if the loot box is in the cargo hold.
		const auto& cargoList = GetUnmountedCargoList(clientId);
		uint lootBoxItemId = 0;
		for (const auto& cargo : cargoList)
		{
			if (cargo.iArchID == lootBox.boxArchetypeId)
			{
				// Limit the boxes to open by how many there actually are in the cargo.
				openCount = std::min(openCount, cargo.iCount);
				lootBoxItemId = cargo.iID;
				break;
			}
		}
		if (!lootBoxItemId)
		{
			PrintUserCmdText(clientId, L"'" + lootBox.boxArchetypeName + L"' is not in your cargo hold!");
			return false;
		}

		// Check if the key is in the cargo hold.
		uint keyItemId = 0;
		if (lootBox.keyArchetypeId)
		{
			for (const auto& cargo : cargoList)
			{
				if (cargo.iArchID == lootBox.keyArchetypeId && cargo.iCount >= openCount)
				{
					keyItemId = cargo.iID;
					break;
				}
			}
		}
		if (lootBox.keyArchetypeId && !keyItemId)
		{
			PrintUserCmdText(clientId, std::to_wstring(openCount) + L" '" + lootBox.boxArchetypeName + L"' cannot be opened without " + std::to_wstring(openCount) + L" '" + lootBox.keyArchetypeName + L"' in your cargo hold!");
			return false;
		}

		// Check if there's enough cargo hold to add the looted item.
		float remainingHoldSize = 0.0f;
		pub::Player::GetRemainingHoldSize(clientId, remainingHoldSize);
		const float requiredHoldSize = std::max(0.0f, (-lootBox.boxArchetypeVolume - lootBox.keyArchetypeVolume + lootBox.highestLootArchetypeVolume) * openCount);
		const float holdSizeDifference = remainingHoldSize - requiredHoldSize;
		if (holdSizeDifference < 0.0f)
		{
			PrintUserCmdText(clientId, L"'" + lootBox.boxArchetypeName + L"' cannot be opened without at least " + std::to_wstring(static_cast<int>(std::ceil(abs(holdSizeDifference)))) + L" more units of space in your cargo hold!");
			return false;
		}

		const std::wstring& characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		// Try to remove the key item from the cargo hold if required.
		if (lootBox.keyArchetypeId)
		{
			if (HkRemoveCargo(characterNameWS, keyItemId, openCount) != HKE_OK)
				return false;
		}

		// Try to remove the loot box item from the cargo hold.
		if (HkRemoveCargo(characterNameWS, lootBoxItemId, openCount) != HKE_OK)
			return false;

		// Finally roll the dice to get the actual looted item.
		// First collect all items to add to cargo. Otherwise too many packages are sent and cause lags!
		std::map<uint, uint> lootedArchetypeIds;
		for (int openedCount = 0; openedCount < openCount; openedCount++)
		{
			const int lootIndex = lootBox.lootArchetypeIdsDistribution(randomizer);
			const uint lootedItemArchetypeId = lootBox.lootArchetypeIds[lootIndex];
			if (!lootedArchetypeIds.contains(lootedItemArchetypeId))
				lootedArchetypeIds[lootedItemArchetypeId] = 0;
			lootedArchetypeIds[lootedItemArchetypeId]++;
			if (openCount < 5)
				PrintUserCmdText(clientId, L"Looted '" + lootBox.lootArchetypeNames[lootIndex] + L"' from '" + lootBox.boxArchetypeName + L"'.");
		}
		// Now add cargo for the amount of items we need.
		for (const auto& lootedItemArchetypeIdCount : lootedArchetypeIds)
		{
			// Items that cannot be stacked must be sent singular. This causes a lot of package traffic!
			if (lootArchetypeCombinable[lootedItemArchetypeIdCount.first])
			{
				for (uint count = 0; count < lootedItemArchetypeIdCount.second; count++)
					HkAddCargo(ARG_CLIENTID(clientId), lootedItemArchetypeIdCount.first, 1, false);
			}
			// Stackable items will be sent as a batch to reduce package traffic.
			else
			{
				HkAddCargo(ARG_CLIENTID(clientId), lootedItemArchetypeIdCount.first, lootedItemArchetypeIdCount.second, false);
			}
		}

		if (openCount >= 5)
			PrintUserCmdText(clientId, L"Looted many items from " + std::to_wstring(openCount) + L" '" + lootBox.boxArchetypeName + L"'.");

		if (successSoundId)
		{
			pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);
			pub::Audio::PlaySoundEffect(clientId, successSoundId);
		}

		return true;
	}

	bool UserCmd_Open(const uint clientId, const std::wstring& argumentsWS)
	{
		if (ToLower(argumentsWS).find(L"/open") == 0)
		{
			const std::wstring& arguments = Trim(GetParamToEnd(argumentsWS, ' ', 1));
			const size_t lastWhiteSpace = arguments.find_last_of(' ');
			int count = ToInt(arguments.substr(lastWhiteSpace + 1));
			std::wstring lootBoxName = arguments;
			if (count != 0)
			{
				lootBoxName = Trim(arguments.substr(0, lastWhiteSpace));
			}
			// Limit the opening of crates to 100 to prevent possible lag issues when sending packages.
			if (!Open(clientId, wstos(lootBoxName), std::min(100, std::max(1, count))))
			{
				if (failSoundId)
					pub::Audio::PlaySoundEffect(clientId, failSoundId);
			}
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		returncode = DEFAULT_RETURNCODE;
		return false;
	}
}