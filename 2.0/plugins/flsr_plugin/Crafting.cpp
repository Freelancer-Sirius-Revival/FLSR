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
	};

	static std::map<std::string, Recipe> recipes;

	static uint successSoundId = 0;
	static uint failSoundId = 0;

	const uint NOT_ENOUGH_MONEY = pub::GetNicknameId("not_enough_money");
	const uint INSUFFICIENT_CARGO_SPACE_ID = pub::GetNicknameId("insufficient_cargo_space");
	const uint LOADED_INTO_CARGO_HOLD_ID = pub::GetNicknameId("loaded_into_cargo_hold");
	const uint NONE_AVAILABLE_ID = pub::GetNicknameId("none_available");

	void LoadSettings()
	{
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
							successSoundId = CreateID(ini.get_value_string(0));
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
					}
					if (recipe.archetypeId && recipe.count && !recipe.ingredientArchetypeIdsWithCount.empty())
					{
						recipes[recipeNameLower] = recipe;
					}
				}
			}
			ini.close();
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

	bool Craft(const uint clientId, const std::string& recipeName, const int batchCount)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		// Check if the recipe exists.
		if (recipeName.empty())
		{
			PrintUserCmdText(clientId, L"Schematic name must be given to craft it!");
			return false;
		}

		std::string recipeNameLower = ToLower(recipeName);
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

		// Check if the required ingredients are in the cargo hold.
		const auto& cargoList = GetUnmountedCargoList(clientId);
		std::vector<std::pair<CARGO_INFO, int>> foundCargosAndRequiredCount;
		for (const auto& ingredientWithCount : recipe.ingredientArchetypeIdsWithCount)
		{
			for (const auto& cargo : cargoList)
			{
				if (cargo.iArchID == ingredientWithCount.first)
				{
					const int multipliedIngredientCount = ingredientWithCount.second * batchCount;
					if (cargo.iCount < multipliedIngredientCount)
						break;
					foundCargosAndRequiredCount.push_back({ cargo, multipliedIngredientCount });
				}
			}
		}

		const std::wstring& characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		if (recipe.cost)
		{
			int currentCash;
			if (HkGetCash(characterNameWS, currentCash) != HKE_OK)
				return false;
			if (currentCash < (recipe.cost * batchCount))
			{
				pub::Player::SendNNMessage(clientId, NOT_ENOUGH_MONEY);
				PrintUserCmdText(clientId, L"Credits are missing to craft " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
				return false;
			}
		}

		if (foundCargosAndRequiredCount.size() != recipe.ingredientArchetypeIdsWithCount.size())
		{
			pub::Player::SendNNMessage(clientId, NONE_AVAILABLE_ID);
			PrintUserCmdText(clientId, L"Parts are missing to craft " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
			return false;
		}

		// Check if there's enough cargo hold to add the produced item.
		const Archetype::Equipment* equipment = Archetype::GetEquipment(recipe.archetypeId);
		if (!equipment)
			return false;
		float totalVolumeNeeded = equipment->fVolume * recipe.count * batchCount;
		for (const auto& foundCargoAndRequiredCount : foundCargosAndRequiredCount)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(foundCargoAndRequiredCount.first.iArchID);
			if (!equipment)
				return false;
			totalVolumeNeeded -= equipment->fVolume * foundCargoAndRequiredCount.second;
		}
		float remainingHoldSize;
		pub::Player::GetRemainingHoldSize(clientId, remainingHoldSize);
		if (remainingHoldSize - totalVolumeNeeded < 0.0f)
		{
			pub::Player::SendNNMessage(clientId, INSUFFICIENT_CARGO_SPACE_ID);
			PrintUserCmdText(clientId, L"Not enough cargo space left to craft " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
			return false;
		}

		// Exchange the items in the cargo hold.
		for (const auto& foundCargoAndRequiredCount : foundCargosAndRequiredCount)
		{
			if (HkRemoveCargo(characterNameWS, foundCargoAndRequiredCount.first.iID, foundCargoAndRequiredCount.second) != HKE_OK)
				return false;
		}

		if (recipe.cost)
			if (HkAddCash(characterNameWS, -recipe.cost * batchCount) != HKE_OK)
				return false;

		if (Tools::FLSRHkAddCargo(characterNameWS, recipe.archetypeId, recipe.count * batchCount, false) == HKE_OK)
		{
			if (successSoundId)
				pub::Audio::PlaySoundEffect(clientId, successSoundId);
			pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);
			PrintUserCmdText(clientId, L"Successfully crafted " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'.");
			return true;
		}
		return false;
	}

	bool UserCmd_Craft(const uint clientId, const std::wstring& argumentsWS)
	{
		returncode = DEFAULT_RETURNCODE;
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
		return false;
	}
}