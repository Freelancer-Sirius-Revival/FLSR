#include "Main.h"
#include <random>

namespace LootBoxes
{
	static std::mt19937 randomizer(std::random_device{}());

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

	static std::unordered_map<std::string, LootBox> lootBoxes;

	static uint successSoundId = 0;
	static uint failSoundId = 0;

	const uint LOADED_INTO_CARGO_HOLD_ID = pub::GetNicknameId("loaded_into_cargo_hold");

	float GetEquipmentVolume(const uint archetypeId)
	{
		const Archetype::Equipment* equipment = Archetype::GetEquipment(archetypeId);
		if (equipment)
			return equipment->fVolume;
		return 0.0f;
	}

	std::wstring GetEquipmentName(const uint archetypeId)
	{
		const GoodInfo* goodInfo = GoodList::find_by_id(archetypeId);
		if (goodInfo)
			return HkGetWStringFromIDS(goodInfo->iIDSName);
		return L"";
	}

	static bool initialized = false;
	void ReadInitialData()
	{
		if (initialized)
			return;
		initialized = true;

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
			ConPrint(L"Module loaded: Loot Boxes\n");
		}
	}

	std::vector<CARGO_INFO> GetUnmountedCargoList(const uint clientId)
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

	bool Open(const uint clientId, const std::string lootBoxName)
	{
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

		// Check if the required loot box and the key are in the cargo hold.
		const auto& cargoList = GetUnmountedCargoList(clientId);
		uint lootBoxItemId = 0;
		uint keyItemId = 0;
		for (const auto& cargo : cargoList)
		{
			if (cargo.iArchID == lootBox.boxArchetypeId)
			{
				lootBoxItemId = cargo.iID;
			}
			else if (cargo.iArchID == lootBox.keyArchetypeId)
			{
				keyItemId = cargo.iID;
			}
			if (lootBoxItemId && (!lootBox.keyArchetypeId || keyItemId))
				break;
		}
		if (!lootBoxItemId)
		{
			PrintUserCmdText(clientId, L"'" + lootBox.boxArchetypeName + L"' is not in your cargo hold!");
			return false;
		}
		if (lootBox.keyArchetypeId && !keyItemId)
		{
			PrintUserCmdText(clientId, L"'" + lootBox.boxArchetypeName + L"' cannot be opened without '" + lootBox.keyArchetypeName + L"' in your cargo hold!");
			return false;
		}

		// Check if there's enough cargo hold to add the looted item.
		float remainingHoldSize = 0.0f;
		pub::Player::GetRemainingHoldSize(clientId, remainingHoldSize);
		const float requiredHoldSize = abs(std::min(0.0f, remainingHoldSize + lootBox.boxArchetypeVolume + lootBox.keyArchetypeVolume - lootBox.highestLootArchetypeVolume));
		if (requiredHoldSize > 0.0f)
		{
			PrintUserCmdText(clientId, L"'" + lootBox.boxArchetypeName + L"' cannot be opened without at least " + std::to_wstring(static_cast<int>(std::ceil(requiredHoldSize))) + L" more units of space in your cargo hold!");
			return false;
		}

		const std::wstring& characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		// Try to remove the key item from the cargo hold if required.
		if (lootBox.keyArchetypeId)
		{
			if (HkRemoveCargo(characterNameWS, keyItemId, 1) != HKE_OK)
				return false;
		}

		// Try to remove the loot box item from the cargo hold.
		if (HkRemoveCargo(characterNameWS, lootBoxItemId, 1) != HKE_OK)
			return false;

		// Finally roll the dice to get the actual looted item.
		const int lootIndex = lootBox.lootArchetypeIdsDistribution(randomizer);
		const uint lootedItemArchetypeId = lootBox.lootArchetypeIds[lootIndex];
		if (Tools::FLSRHkAddCargo(characterNameWS, lootedItemArchetypeId, 1, false) == HKE_OK)
		{
			if (successSoundId)
				pub::Audio::PlaySoundEffect(clientId, successSoundId);
			pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);
			PrintUserCmdText(clientId, L"Looted '" + lootBox.lootArchetypeNames[lootIndex] + L"' from '" + lootBox.boxArchetypeName + L"'.");
			return true;
		}
		return false;
	}

	bool UserCmd_Open(const uint clientId, const std::wstring& argumentsWS)
	{
		returncode = DEFAULT_RETURNCODE;
		if (ToLower(argumentsWS).find(L"/open") == 0)
		{
			const std::wstring& arguments = Trim(GetParamToEnd(argumentsWS, ' ', 1));
			const std::wstring lootBoxName = Trim(arguments);
			if (!Open(clientId, wstos(lootBoxName)))
			{
				if (failSoundId)
					pub::Audio::PlaySoundEffect(clientId, failSoundId);
			}
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		return false;
	}
}