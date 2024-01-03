#include "main.h"

/**
* [Foo Bar]
* product = item_nickname, count
* ingredient = item_nickname, count
* ...
* base_nickname = base_name
* ...
* 
*/

namespace Crafting
{
	struct Recipe
	{
		std::wstring originalName = L"";
		uint archetypeId = 0;
		int count = 0;
		std::vector<std::pair<uint, int>> ingredientArchetypeIdsWithCount;
		std::set<uint> validBaseIds;
	};

	static std::map<std::string, Recipe> recipes;

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
			ini.close();
		}
	}

    std::vector<CARGO_INFO> GetUnmountedCargoList(uint clientId)
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

	void Craft(uint clientId, std::string recipeName, int count)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		// Check if the recipe exists.
		if (recipeName.empty())
		{
			PrintUserCmdText(clientId, L"Schematic name must be given to craft it!");
			return;
		}

		std::string recipeNameLower = ToLower(recipeName);
		if (!recipes.contains(recipeNameLower))
		{
			PrintUserCmdText(clientId, L"Schematic '" + stows(recipeName) + L"' does not exist!");
			return;
		}
		const auto& recipe = recipes[recipeNameLower];

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
					return;
				}
			}
		}
		else
		{
			PrintUserCmdText(clientId, L"You must be docked to craft items!");
			return;
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
					const int multipliedIngredientCount = ingredientWithCount.second * count;
					if (cargo.iCount < multipliedIngredientCount)
						break;
					foundCargosAndRequiredCount.push_back({ cargo, multipliedIngredientCount });
				}
			}
		}
		if (foundCargosAndRequiredCount.size() != recipe.ingredientArchetypeIdsWithCount.size())
		{
			PrintUserCmdText(clientId, L"Parts are missing to craft " + std::to_wstring(count) + L" '" + recipe.originalName + L"'!");
			return;
		}

		// Check if there's enough cargo hold to add the produced item.
		const Archetype::Equipment* equipment = Archetype::GetEquipment(recipe.archetypeId);
		if (!equipment)
			return;
		float totalVolumeNeeded = equipment->fVolume * count;
		for (const auto& foundCargoAndRequiredCount : foundCargosAndRequiredCount)
		{
			const Archetype::Equipment* equipment = Archetype::GetEquipment(foundCargoAndRequiredCount.first.iArchID);
			if (!equipment)
				return;
			totalVolumeNeeded -= equipment->fVolume * foundCargoAndRequiredCount.second;
		}
		float remainingHoldSize;
		pub::Player::GetRemainingHoldSize(clientId, remainingHoldSize);
		if (remainingHoldSize - totalVolumeNeeded < 0.0f)
		{
			PrintUserCmdText(clientId, L"Not enough cargo space left to craft " + std::to_wstring(count) + L" '" + recipe.originalName + L"'!");
			return;
		}

		// Exchange the items in the cargo hold.
		const std::wstring& characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		for (const auto& foundCargoAndRequiredCount : foundCargosAndRequiredCount)
		{
			if (HkRemoveCargo(characterNameWS, foundCargoAndRequiredCount.first.iID, foundCargoAndRequiredCount.second) != HKE_OK)
				return;
		}
		if (Tools::FLSRHkAddCargo(characterNameWS, recipe.archetypeId, count, false) == HKE_OK)
		{
			PrintUserCmdText(clientId, L"Successfully crafted " + std::to_wstring(count) + L" '" + recipe.originalName + L"'.");
		}
	}

	bool UserCmd_Craft(uint clientId, const std::wstring& argumentsWS)
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
				Craft(clientId, wstos(recipeName), std::min(1000, std::max(1, count)));
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}
		}
		return false;
	}
}