#include "Crafting.h"
#include "Plugin.h"

/**
* [General]
* success_sound_nickname = 
* fail_sound_nickname = 
* 
* [Foo Bar]
* product = good_nickname, count
* cost = money
* ingredient = good_nickname, count
* ...
* base_nickname = base_name
* ...
* ship_nickname = shiparch_name
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
		std::set<uint> validShipIds;
		uint successSoundId = 0;
	};

	std::unordered_map<std::string, Recipe> recipes;
	std::unordered_map<uint, std::wstring> currentShipCraftingPopups;

	uint defaultSuccessSoundId = 0;
	uint failSoundId = 0;

	const uint NOT_ENOUGH_MONEY = pub::GetNicknameId("not_enough_money");
	const uint INSUFFICIENT_CARGO_SPACE_ID = pub::GetNicknameId("insufficient_cargo_space");
	const uint LOADED_INTO_CARGO_HOLD_ID = pub::GetNicknameId("loaded_into_cargo_hold");
	const uint ALL_SYSTEMS_NORMAL_ID = pub::GetNicknameId("all_systems_normal");
	const uint NONE_AVAILABLE_ID = pub::GetNicknameId("none_available");

	void LoadSettings()
	{
		ConPrint(L"Initializing Crafting... ");

		HkLoadStringDLLs();

		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-Crafting.cfg";

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
							if (ini.get_num_parameters() > 1)
								recipe.count = ini.get_value_int(1);
							else
								recipe.count = 1;
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
						else if (ini.is_value("ship_nickname"))
						{
							recipe.validShipIds.insert(CreateID(ini.get_value_string(0)));
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

	static std::wstring PrintMoney(const int64_t amount)
	{
		std::wstring result = std::to_wstring(amount);
		for (int pos = result.size() - 3; pos > 0; pos = pos - 3)
			result = result.insert(pos, L",");
		return L"$" + result;
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

	static Recipe* FindRecipe(const uint clientId, const std::string& recipeName)
	{
		// Check if the recipe exists.
		if (recipeName.empty())
		{
			PrintUserCmdText(clientId, L"Schematic name must be given to craft it!");
			return nullptr;
		}

		const std::string recipeNameLower = ToLower(recipeName);
		if (!recipes.contains(recipeNameLower))
		{
			PrintUserCmdText(clientId, L"Schematic '" + stows(recipeName) + L"' does not exist!");
			return nullptr;
		}
		return &recipes[recipeNameLower];
	}

	static int CorrectBatchCount(const uint clientId, const Recipe& recipe, int batchCount)
	{
		if (batchCount < 1)
			batchCount = 1;

		const GoodInfo* productGoodInfo = GoodList::find_by_id(recipe.archetypeId);
		if (!productGoodInfo)
		{
			PrintUserCmdText(clientId, L"Internal Good Nickname not found for '" + recipe.originalName + L"'!");
			return 0;
		}

		if (productGoodInfo->type == GoodInfo::GoodType::Hull)
		{
			PrintUserCmdText(clientId, recipe.originalName + L" is a ship hull and cannot be crafted. Contact an admin!");
			return 0;
		}

		if (productGoodInfo->type == GoodInfo::GoodType::Ship)
		{
			return 1;
		}
		
		return batchCount;
	}

	static bool TestBaseRequirements(const uint clientId, const Recipe& recipe)
	{
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
		return true;
	}

	static bool TestShipRequirements(const uint clientId, const Recipe& recipe)
	{
		if (!recipe.validShipIds.empty())
		{
			uint shipArchetypeId;
			pub::Player::GetShipID(clientId, shipArchetypeId);
			if (!recipe.validShipIds.contains(shipArchetypeId))
			{
				PrintUserCmdText(clientId, L"You must own a specific ship to craft '" + recipe.originalName + L"'!");
				return false;
			}
		}
		return true;
	}

	static bool TestCashRequirements(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		if (recipe.cost)
		{
			int currentCash;
			const std::wstring& characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
			if (HkGetCash(characterNameWS, currentCash) != HKE_OK)
				return false;
			const int totalCost = recipe.cost * batchCount;
			const int cashDiff = currentCash - totalCost;
			if (cashDiff < 0)
			{
				pub::Player::SendNNMessage(clientId, NOT_ENOUGH_MONEY);
				PrintUserCmdText(clientId, PrintMoney(-cashDiff) + L" missing to craft " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
				return false;
			}
		}
		return true;
	}

	static bool TestCargoSpaceRequirements(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		const GoodInfo* productGoodInfo = GoodList::find_by_id(recipe.archetypeId);
		if (!productGoodInfo)
			return false;

		if (productGoodInfo->type != GoodInfo::GoodType::Ship)
		{
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
		}

		return true;
	}

	static bool TestIngredientsRequirements(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		const auto& cargoList = GetUnmountedCargoList(clientId);
		std::vector<std::pair<uint, int>> missingIngredientArchetypeIdsWithCount;
		// Copy the ingredients and their count.
		for (const auto& ingredientWithCount : recipe.ingredientArchetypeIdsWithCount)
			missingIngredientArchetypeIdsWithCount.push_back({ ingredientWithCount.first, ingredientWithCount.second * batchCount });

		for (auto& ingredientWithCount : missingIngredientArchetypeIdsWithCount)
		{
			for (const CARGO_INFO& cargo : cargoList)
			{
				if (cargo.iArchID == ingredientWithCount.first)
				{
					uint oldRequiredCount = ingredientWithCount.second;
					ingredientWithCount.second = std::max<int>(ingredientWithCount.second - cargo.iCount, 0);
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

		return true;
	}

	static bool TestAllRequirements(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		return	TestBaseRequirements(clientId, recipe) &&
				TestShipRequirements(clientId, recipe) &&
				TestCashRequirements(clientId, recipe, batchCount) &&
				TestCargoSpaceRequirements(clientId, recipe, batchCount) &&
				TestIngredientsRequirements(clientId, recipe, batchCount);
	}

	static bool ConsumeAllIngredients(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		const auto& cargoList = GetUnmountedCargoList(clientId);
		// Copy the ingredients and their count.
		std::vector<std::pair<uint, int>> missingIngredientArchetypeIdsWithCount;
		for (const auto& ingredientWithCount : recipe.ingredientArchetypeIdsWithCount)
			missingIngredientArchetypeIdsWithCount.push_back({ ingredientWithCount.first, ingredientWithCount.second * batchCount });

		// Collect individual cargo IDs and the count of their actual cargo. If multiple of a single equipment arch are needed, it will be split across multiple cargo items.
		std::unordered_map<uint, std::vector<std::pair<uint, uint>>> foundIngredientIdsWithCountByArchetypeId;
		for (auto& ingredientWithCount : missingIngredientArchetypeIdsWithCount)
		{
			for (const CARGO_INFO& cargo : cargoList)
			{
				if (cargo.iArchID == ingredientWithCount.first)
				{
					uint oldRequiredCount = ingredientWithCount.second;
					ingredientWithCount.second = std::max<int>(ingredientWithCount.second - cargo.iCount, 0);
					foundIngredientIdsWithCountByArchetypeId[cargo.iArchID].push_back({ cargo.iID, oldRequiredCount - ingredientWithCount.second });
				}
				if (ingredientWithCount.second <= 0)
					break;
			}
			if (ingredientWithCount.second <= 0)
				continue;
		}

		// Remove all collected cargo ids and the required count.
		const std::wstring& characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
		for (const auto& ingredientArchWithIdsAndCount : foundIngredientIdsWithCountByArchetypeId)
		{
			for (const auto& cargoIdWithCount : ingredientArchWithIdsAndCount.second)
			{
				if (HkRemoveCargo(characterNameWS, cargoIdWithCount.first, cargoIdWithCount.second) != HKE_OK)
					return false;
			}
		}

		// Consume money.
		if (recipe.cost)
		{
			if (HkAddCash(characterNameWS, -recipe.cost * batchCount) != HKE_OK)
				return false;
		}

		return true;
	}

	static bool ProduceItems(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		if (HkAddCargo(ARG_CLIENTID(clientId), recipe.archetypeId, recipe.count * batchCount, false) != HKE_OK)
			return false;

		const uint soundId = recipe.successSoundId ? recipe.successSoundId : defaultSuccessSoundId;
		if (soundId)
			pub::Audio::PlaySoundEffect(clientId, soundId);
		pub::Player::SendNNMessage(clientId, LOADED_INTO_CARGO_HOLD_ID);
		std::wstring message = L"Successfully crafted " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'";
		if (recipe.cost)
			message += L" for " + PrintMoney(recipe.cost * batchCount);
		PrintUserCmdText(clientId, message + L".");
		return true;
	}

	static bool ProduceShip(const uint clientId, const Recipe& recipe, const GoodInfo* shipGood)
	{
		const GoodInfo* hullGood = GoodList::find_by_id(shipGood->iHullGoodID);
		if (!hullGood)
			return false;

		EquipDescVector newEquip(shipGood->edl);
		for (const auto& equip : Players[clientId].equipDescList.equip)
		{
			switch (Archetype::GetEquipment(equip.iArchID)->get_class_type())
			{
				case Archetype::COMMODITY:
				case Archetype::POWER:
				case Archetype::ENGINE:
				case Archetype::SHIELD_GENERATOR:
				case Archetype::THRUSTER:
				case Archetype::LAUNCHER:
				case Archetype::GUN:
				case Archetype::MINE_DROPPER:
				case Archetype::MINE:
				case Archetype::COUNTER_MEASURE_DROPPER:
				case Archetype::COUNTER_MEASURE:
				case Archetype::REPAIR_KIT:
				case Archetype::SHIELD_BATTERY:
				case Archetype::MUNITION:
				{
					EquipDesc equipCopy(equip);
					equipCopy.bMounted = false;
					equipCopy.szHardPoint.value = EquipDesc::CARGO_BAY_HP_NAME.value;
					newEquip.equip.push_back(equipCopy);
				}
			}
		}

		pub::Player::SetShipAndLoadout(clientId, hullGood->shipArchId, newEquip);
		const uint soundId = recipe.successSoundId ? recipe.successSoundId : defaultSuccessSoundId;
		if (soundId)
			pub::Audio::PlaySoundEffect(clientId, soundId);
		pub::Player::SendNNMessage(clientId, ALL_SYSTEMS_NORMAL_ID);
		std::wstring message = L"Successfully crafted '" + recipe.originalName + L"'";
		return true;
	}

	static bool Craft(const uint clientId, const std::string& recipeName, int batchCount)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		const Recipe* recipe = FindRecipe(clientId, recipeName);
		if (!recipe)
			return false;

		batchCount = CorrectBatchCount(clientId, *recipe, batchCount);
		if (batchCount == 0)
			return false;

		const GoodInfo* good = GoodList::find_by_id(recipe->archetypeId);
		if (!good)
			return false;

		bool result = TestAllRequirements(clientId, *recipe, batchCount);
		if (result)
		{
			if (good->type != GoodInfo::GoodType::Ship)
			{
				result = ConsumeAllIngredients(clientId, *recipe, batchCount) && ProduceItems(clientId, *recipe, batchCount);
			}
			else
			{
				if (const auto& entry = currentShipCraftingPopups.find(clientId); entry != currentShipCraftingPopups.end() && entry->second == recipe->originalName)
				{
					currentShipCraftingPopups.erase(entry);
					result = ConsumeAllIngredients(clientId, *recipe, batchCount) && ProduceShip(clientId, *recipe, good);
				}
				else
				{
					currentShipCraftingPopups.erase(clientId);

					const GoodInfo* hullGood = GoodList::find_by_id(good->iHullGoodID);
					if (!hullGood || !hullGood->shipArchId)
						return false;

					currentShipCraftingPopups.insert({ clientId, recipe->originalName });
					pub::Player::PopUpDialog(clientId, FmtStr(524393, 0), FmtStr(524394, 0), PopupDialogButton::LEFT_YES | PopupDialogButton::RIGHT_LATER);
				}
			}
		}

		if (result)
			return true;

		if (failSoundId)
			pub::Audio::PlaySoundEffect(clientId, failSoundId);
		return false;
	}

	bool UserCmd_Craft(const uint clientId, const std::wstring& argumentsWS)
	{
		if (ToLower(argumentsWS).find(L"/craft") == 0)
		{
			const std::wstring& arguments = Trim(GetParamToEnd(argumentsWS, ' ', 1));
			const size_t lastWhiteSpace = arguments.find_last_of(' ');
			int count = ToInt(arguments.substr(lastWhiteSpace + 1));
			std::wstring recipeName = arguments;
			if (count != 0)
				recipeName = Trim(arguments.substr(0, lastWhiteSpace));
			currentShipCraftingPopups.erase(clientId); // Make sure to not skip the popup this way.
			Craft(clientId, ToLower(wstos(recipeName)), std::min<int>(1000, std::max<int>(1, count)));
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		returncode = DEFAULT_RETURNCODE;
		return false;
	}

	void __stdcall PopUpDialog(unsigned int clientId, unsigned int buttonClicked)
	{
		if (const auto& entry = currentShipCraftingPopups.find(clientId); entry != currentShipCraftingPopups.end())
		{
			if (buttonClicked == PopupDialogButton::LEFT_YES)
				Craft(clientId, ToLower(wstos(entry->second)), 1);
			else
				currentShipCraftingPopups.erase(entry);
			returncode = SKIPPLUGINS;
			return;
		}
		returncode = DEFAULT_RETURNCODE;
	}
}