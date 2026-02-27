#include "main.h"

/**
* [General]
* success_sound_nickname = 
* fail_sound_nickname = 
* 
* [Foo Bar]
* product = item_nickname, count
* cost = money
* ingredient = item_nickname, count
* ...
* base_nickname = base_name
* ...
*/

namespace Crafting
{
	struct Recipe
	{
		std::wstring originalName = L"";
		uint archetypeId = 0;
		int count = 0;
		int cost = 0;
		std::vector<std::pair<uint, int>> ingredientArchetypeIdsWithCount;
		std::set<uint> validBaseIds;
		uint successSoundId = 0;
	};

	std::unordered_map<std::string, Recipe> recipes;

	uint defaultSuccessSoundId = 0;
	uint failSoundId = 0;

	const uint NOT_ENOUGH_MONEY = pub::GetNicknameId("not_enough_money");
	const uint INSUFFICIENT_CARGO_SPACE_ID = pub::GetNicknameId("insufficient_cargo_space");
	const uint LOADED_INTO_CARGO_HOLD_ID = pub::GetNicknameId("loaded_into_cargo_hold");
	const uint NONE_AVAILABLE_ID = pub::GetNicknameId("none_available");

	void LoadSettings()
	{
		ConPrint(L"Initializing Crafting... ");

		HkLoadStringDLLs();

		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + Globals::CRAFTING_CONFIG_FILE;

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
							defaultSuccessSoundId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("fail_sound_nickname"))
						{
							failSoundId = CreateID(ini.get_value_string(0));
						}
					}
				}
				else
				{
					Recipe recipe;
					recipe.originalName = stows(ini.get_header_ptr());
					std::string recipeNameLower = ToLower(wstos(recipe.originalName));
					while (ini.read_value())
					{
						if (ini.is_value("product"))
						{
							recipe.archetypeId = CreateID(ini.get_value_string(0));
							recipe.count = ini.get_value_int(1);
						}
						else if (ini.is_value("cost"))
						{
							recipe.cost = ini.get_value_int(0);
						}
						else if (ini.is_value("ingredient"))
						{
							recipe.ingredientArchetypeIdsWithCount.push_back({ CreateID(ini.get_value_string(0)), ini.get_value_int(1) });
						}
						else if (ini.is_value("base_nickname"))
						{
							recipe.validBaseIds.insert(CreateID(ini.get_value_string(0)));
						}
						else if (ini.is_value("success_sound_nickname"))
						{
							recipe.successSoundId = CreateID(ini.get_value_string(0));
						}
					}
					if (recipe.archetypeId && recipe.count && !recipe.ingredientArchetypeIdsWithCount.empty())
					{
						recipes[recipeNameLower] = recipe;
					}
				}
			}
			ini.close();
		}

		ConPrint(L"Done\n");
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

	static bool Craft(const uint clientId, const std::string& recipeName, int batchCount)
	{
		if (batchCount < 1)
			batchCount = 1;

		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		// Check if the recipe exists.
		if (recipeName.empty())
		{
			PrintUserCmdText(clientId, L"Schematic name must be given to craft it!");
			return false;
		}

		const std::string recipeNameLower = ToLower(recipeName);
		if (!recipes.contains(recipeNameLower))
		{
			PrintUserCmdText(clientId, L"Schematic '" + stows(recipeName) + L"' does not exist!");
			return false;
		}
		const Recipe& recipe = recipes[recipeNameLower];

		// Check if the player is docked on the requested base.
		uint shipId;
		pub::Player::GetShip(clientId, shipId);
		if (!shipId)
		{
			if (!recipe.validBaseIds.empty())
			{
				uint baseId;
				pub::Player::GetBase(clientId, baseId);
				if (!recipe.validBaseIds.contains(baseId))
				{
					PrintUserCmdText(clientId, L"You must be docked on a specific base to craft '" + recipe.originalName + L"'!");
					return false;
				}
			}
		}
		else
		{
			PrintUserCmdText(clientId, L"You must be docked to craft items!");
			return false;
		}

		const std::wstring& characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		if (recipe.cost)
		{
			int currentCash;
			if (HkGetCash(characterNameWS, currentCash) != HKE_OK)
				return false;
			const int totalCost = recipe.cost * batchCount;
			const int cashDiff = currentCash - totalCost;
			if (cashDiff < 0)
			{
				pub::Player::SendNNMessage(clientId, NOT_ENOUGH_MONEY);
				PrintUserCmdText(clientId, L"$" + ToMoneyStr(-cashDiff) + L" missing to craft " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
				return false;
			}
		}

		// Check if the required ingredients are in the cargo hold.
		const auto& cargoList = GetUnmountedCargoList(clientId);
		std::vector<std::pair<uint, int>> missingIngredientArchetypeIdsWithCount;
		// Copy the ingredients and their count.
		for (const auto& ingredientWithCount : recipe.ingredientArchetypeIdsWithCount)
			missingIngredientArchetypeIdsWithCount.push_back({ ingredientWithCount.first, ingredientWithCount.second * batchCount });

		std::unordered_map<uint, std::vector<std::pair<uint, uint>>> foundIngredientIdsWithCountByArchetypeId;

		for (auto& ingredientWithCount : missingIngredientArchetypeIdsWithCount)
		{
			for (const CARGO_INFO& cargo : cargoList)
			{
				if (cargo.iArchID == ingredientWithCount.first)
				{
					uint oldRequiredCount = ingredientWithCount.second;
					ingredientWithCount.second = std::max(ingredientWithCount.second - cargo.iCount, 0);
					foundIngredientIdsWithCountByArchetypeId[cargo.iArchID].push_back({ cargo.iID, oldRequiredCount - ingredientWithCount.second });
				}
				if (ingredientWithCount.second <= 0)
					break;
			}
			if (ingredientWithCount.second <= 0)
				continue;
		}

		std::vector<std::wstring> missingPartsTexts;
		for (const auto& ingredientWithCount : missingIngredientArchetypeIdsWithCount)
		{
			if (ingredientWithCount.second > 0)
			{
				const GoodInfo* goodInfo = GoodList::find_by_id(ingredientWithCount.first);
				if (goodInfo)
					missingPartsTexts.push_back(std::to_wstring(ingredientWithCount.second) + L" " + HkGetWStringFromIDS(goodInfo->iIDSName));
				else
					missingPartsTexts.push_back(L"");
			}
		}

		if (missingPartsTexts.size() > 0)
		{
			pub::Player::SendNNMessage(clientId, NONE_AVAILABLE_ID);
			std::wstring joinedMissingPartsText = L"";
			for (uint index = 0, length = missingPartsTexts.size(); index < length; index++)
			{
				joinedMissingPartsText += missingPartsTexts[index];
				if (index + 1 < length)
					joinedMissingPartsText += L", ";
			}
			PrintUserCmdText(clientId, joinedMissingPartsText + L" missing to craft " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
			return false;
		}

		// Check if there's enough cargo hold to add the produced item.
		const Archetype::Equipment* equipment = Archetype::GetEquipment(recipe.archetypeId);
		if (!equipment)
			return false;
		float totalVolumeNeeded = equipment->fVolume * recipe.count * batchCount;
		for (const auto& ingredientWithCount : recipe.ingredientArchetypeIdsWithCount)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(ingredientWithCount.first);
			if (!equipment)
				return false;
			totalVolumeNeeded -= equipment->fVolume * ingredientWithCount.second * batchCount;
		}
		float remainingHoldSize;
		pub::Player::GetRemainingHoldSize(clientId, remainingHoldSize);
		float cargoHoldDiff = remainingHoldSize - totalVolumeNeeded;
		if (cargoHoldDiff < 0.0f)
		{
			pub::Player::SendNNMessage(clientId, INSUFFICIENT_CARGO_SPACE_ID);
			PrintUserCmdText(clientId, std::to_wstring(-cargoHoldDiff) + L" units of cargo space missing to craft " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
			return false;
		}

		// Exchange the items in the cargo hold.
		for (const auto& ingredientWithCount : recipe.ingredientArchetypeIdsWithCount)
		{
			const uint archetypeId = ingredientWithCount.first;
			if (!foundIngredientIdsWithCountByArchetypeId.contains(archetypeId))
				return false;

			for (const auto& ingredientIdWithCount : foundIngredientIdsWithCountByArchetypeId[archetypeId])
				if (HkRemoveCargo(characterNameWS, ingredientIdWithCount.first, ingredientIdWithCount.second) != HKE_OK)
					return false;
		}

		if (recipe.cost)
			if (HkAddCash(characterNameWS, -recipe.cost * batchCount) != HKE_OK)
				return false;

		if (HkAddCargo(ARG_CLIENTID(clientId), recipe.archetypeId, recipe.count * batchCount, false) == HKE_OK)
		{
			const uint soundId = recipe.successSoundId ? recipe.successSoundId : defaultSuccessSoundId;
			if (soundId)
				pub::Audio::PlaySoundEffect(clientId, soundId);
			pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);
			PrintUserCmdText(clientId, L"Successfully crafted " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'.");
			return true;
		}
		return false;
	}

	bool UserCmd_Craft(const uint clientId, const std::wstring& argumentsWS)
	{
		if (Modules::GetModuleState("Crafting"))
		{
			if (ToLower(argumentsWS).find(L"/craft") == 0)
			{
				const std::wstring& arguments = Trim(GetParamToEnd(argumentsWS, ' ', 1));
				const size_t lastWhiteSpace = arguments.find_last_of(' ');
				int count = ToInt(arguments.substr(lastWhiteSpace + 1));
				std::wstring recipeName = arguments;
				if (count != 0)
				{
					recipeName = Trim(arguments.substr(0, lastWhiteSpace));
				}
				if (!Craft(clientId, wstos(recipeName), std::min(1000, std::max(1, count))))
				{
					if (failSoundId)
						pub::Audio::PlaySoundEffect(clientId, failSoundId);
				}
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}
		}
		returncode = DEFAULT_RETURNCODE;
		return false;
	}
}