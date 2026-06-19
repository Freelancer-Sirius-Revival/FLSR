#include "Crafting.h"
#include "Plugin.h"
#include <filesystem>
#include <regex>
#include <random>

/**
* Reads all files in \flhook_plugins\crafting\
* 
* Commands by type: /craft, /dismantle, /loot
* 
* [General]
* success_sound_nickname = 
* fail_sound_nickname = 
* 
* [Foo Bar]
* type = produce/dismantle/loot
* product = good_nickname, minCount, maxCount, weightedChance
* ...
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
	std::mt19937 randomizer(std::random_device{}());

	struct Product
	{
		uint archetypeId = 0;
		bool ship = false;
		int minCount = 1;
		int maxCount = 1;
	};

	struct Ingredient
	{
		uint archetypeId = 0;
		int count = 1;
	};

	enum class RecipeType
	{
		Any,
		Produce,
		Dismantle,
		Loot
	};

	struct Recipe
	{
		RecipeType type = RecipeType::Any;
		std::wstring originalName = L"";
		std::vector<Product> products;
		std::discrete_distribution<int> productDistribution;
		float highestProductVolumeWithMaxCount = 0.0f;
		int shipsCount = 0;
		int cost = 0;
		std::vector<Ingredient> ingredients;
		float totalIngredientsVolume = 0.0f;
		std::set<uint> validBaseIds;
		std::set<uint> validShipIds;
		uint successSoundId = 0;
	};

	std::unordered_map<std::string, Recipe> recipes;
	std::unordered_set<uint> productArchetypeCombinable;
	std::unordered_map<uint, std::wstring> productNamesByArchetypeId;
	std::unordered_map<uint, std::wstring> currentShipCraftingPopups;

	uint defaultSuccessSoundId = 0;
	uint failSoundId = 0;

	const uint NOT_ENOUGH_MONEY = pub::GetNicknameId("not_enough_money");
	const uint INSUFFICIENT_CARGO_SPACE_ID = pub::GetNicknameId("insufficient_cargo_space");
	const uint LOADED_INTO_CARGO_HOLD_ID = pub::GetNicknameId("loaded_into_cargo_hold");
	const uint ALL_SYSTEMS_NORMAL_ID = pub::GetNicknameId("all_systems_normal");
	const uint NONE_AVAILABLE_ID = pub::GetNicknameId("none_available");

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
			return HkGetWStringFromIDS(goodInfo->iIDSName);
		return L"";
	}

	static bool IsShip(const uint archetypeId)
	{
		const GoodInfo* productGoodInfo = GoodList::find_by_id(archetypeId);
		return productGoodInfo && productGoodInfo->type == GoodInfo::GoodType::Ship;
	}

	bool initialized = false;
	void ReadInitialData()
	{
		if (initialized)
			return;
		initialized = true;

		ConPrint(L"Initializing Crafting... ");

		HkLoadStringDLLs();

		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string craftingDirectory = std::string(currentDirectory) + "\\flhook_plugins\\crafting\\";
		if (!std::filesystem::is_directory(craftingDirectory))
			return;

		const std::regex filePattern(".+\\.ini", std::regex_constants::ECMAScript | std::regex_constants::icase);
		for (const auto& entry : std::filesystem::recursive_directory_iterator(craftingDirectory))
		{
			const std::string fileName = wstos(entry.path().filename());
			INI_Reader ini;
			if (std::regex_match(fileName, filePattern) && ini.open((craftingDirectory + fileName).c_str(), false))
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
						std::vector<int> weights;
						recipe.originalName = stows(ini.get_header_ptr());
						std::string recipeNameLower = ToLower(wstos(recipe.originalName));
						while (ini.read_value())
						{
							if (ini.is_value("type"))
							{
								const auto& value = ToLower(std::string(ini.get_value_string(0)));
								if (value == "produce")
									recipe.type = RecipeType::Produce;
								else if (value == "dismantle")
									recipe.type = RecipeType::Dismantle;
								else if (value == "loot")
									recipe.type = RecipeType::Loot;
							}
							else if (ini.is_value("product"))
							{
								Product product;
								product.archetypeId = CreateID(ini.get_value_string(0));
								if (ini.get_num_parameters() > 1)
								{
									product.minCount = std::max<int>(1, ini.get_value_int(1));
									product.maxCount = product.minCount;
								}
								if (ini.get_num_parameters() > 2)
									product.maxCount = std::max<int>(product.minCount, ini.get_value_int(2));
								if (ini.get_num_parameters() > 3)
									weights.push_back(std::max<int>(0, ini.get_value_int(3)));

								product.ship = IsShip(product.archetypeId);
								if (product.ship)
									recipe.shipsCount++;
								else
									recipe.highestProductVolumeWithMaxCount = std::max<float>(recipe.highestProductVolumeWithMaxCount, GetEquipmentVolume(product.archetypeId) * product.maxCount);
								productNamesByArchetypeId.insert({ product.archetypeId, GetEquipmentName(product.archetypeId) });
								const GoodInfo* goodInfo = GoodList::find_by_id(product.archetypeId);
								if (goodInfo && goodInfo->multiCount)
									productArchetypeCombinable.insert(product.archetypeId);

								recipe.products.push_back(product);
							}
							else if (ini.is_value("cost"))
							{
								recipe.cost = ini.get_value_int(0);
							}
							else if (ini.is_value("ingredient"))
							{
								Ingredient ingredient;
								ingredient.archetypeId = CreateID(ini.get_value_string(0));
								if (ini.get_num_parameters() > 1)
									ingredient.count = std::max<int>(1, ini.get_value_int(1));
								recipe.ingredients.push_back(ingredient);
								recipe.totalIngredientsVolume += GetEquipmentVolume(ingredient.archetypeId) * ingredient.count;
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
						if (!recipe.products.empty() && !recipe.ingredients.empty())
						{
							recipe.productDistribution = std::discrete_distribution<int>({ weights.begin(), weights.end() });
							recipes[recipeNameLower] = recipe;
						}
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
			PrintUserCmdText(clientId, L"Scheme name must be given to use it!");
			return nullptr;
		}

		const std::string recipeNameLower = ToLower(recipeName);
		if (!recipes.contains(recipeNameLower))
		{
			PrintUserCmdText(clientId, L"Scheme '" + stows(recipeName) + L"' does not exist!");
			return nullptr;
		}
		return &recipes[recipeNameLower];
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
					PrintUserCmdText(clientId, L"You must be docked on a specific base to use '" + recipe.originalName + L"'!");
					return false;
				}
			}
		}
		else
		{
			PrintUserCmdText(clientId, L"You must be docked to use '" + recipe.originalName + L"'!");
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
				PrintUserCmdText(clientId, L"You must own a specific ship to use '" + recipe.originalName + L"'!");
				return false;
			}
		}
		return true;
	}

	static int CorrectBatchCount(const uint clientId, const Recipe& recipe, int batchCount)
	{
		if (recipe.shipsCount > 0)
		{
			if (batchCount > 1)
				PrintUserCmdText(clientId, L"Batching does not work when there is a possibility to obtain a ship.");
			return 1;
		}
		return batchCount < 1 ? 1 : batchCount;
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
				PrintUserCmdText(clientId, PrintMoney(-cashDiff) + L" missing to use " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
				return false;
			}
		}
		return true;
	}

	static bool TestCargoSpaceRequirements(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		const float requiredHoldSize = std::max<float>(0.0f, (recipe.highestProductVolumeWithMaxCount - recipe.totalIngredientsVolume) * batchCount);
		float remainingHoldSize;
		pub::Player::GetRemainingHoldSize(clientId, remainingHoldSize);
		float cargoHoldDiff = remainingHoldSize - requiredHoldSize;
		if (cargoHoldDiff < 0.0f)
		{
			pub::Player::SendNNMessage(clientId, INSUFFICIENT_CARGO_SPACE_ID);
			PrintUserCmdText(clientId, std::to_wstring(-cargoHoldDiff) + L" units of cargo space missing to use " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
			return false;
		}
		return true;
	}

	static bool TestIngredientsRequirements(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		const auto& cargoList = GetUnmountedCargoList(clientId);
		std::vector<std::pair<uint, int>> missingIngredientArchetypeIdsWithCount;
		// Copy the ingredients and their count.
		for (const auto& ingredient : recipe.ingredients)
			missingIngredientArchetypeIdsWithCount.push_back({ ingredient.archetypeId, ingredient.count * batchCount });

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
			PrintUserCmdText(clientId, joinedMissingPartsText + L" missing to use " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'!");
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
		for (const auto& ingredient : recipe.ingredients)
			missingIngredientArchetypeIdsWithCount.push_back({ ingredient.archetypeId, ingredient.count * batchCount });

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

	static bool ProduceShip(const uint clientId, const Recipe& recipe, const uint shipGoodId)
	{
		const GoodInfo* shipGood = GoodList::find_by_id(shipGoodId);
		if (!shipGood)
			return false;

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
		return true;
	}

	static bool ProduceItems(const uint clientId, const Recipe& recipe, const int batchCount)
	{
		// Finally roll the dice to get the actual crafted item.
		// First collect all items to add to cargo. Otherwise too many packages are sent and cause lags!
		std::map<uint, uint> producedArchetypeIds;
		uint shipGoodId = 0;
		for (int producedCount = 0; producedCount < batchCount; producedCount++)
		{
			const Product& product = recipe.products.at(recipe.productDistribution(randomizer));
			if (product.ship)
			{
				shipGoodId = product.archetypeId;
				ProduceShip(clientId, recipe, product.archetypeId);
				break; // Whenever a ship is crafted, the batch count must be exactly 1.
			}
			if (!producedArchetypeIds.contains(product.archetypeId))
				producedArchetypeIds[product.archetypeId] = 0;
			producedArchetypeIds[product.archetypeId] += std::uniform_int_distribution<>(product.minCount, product.maxCount)(randomizer);
		}
		// Now add cargo for the amount of items we need.
		for (const auto& craftedItemArchetypeIdCount : producedArchetypeIds)
		{
			// Items that cannot be stacked must be sent singular. This causes a lot of package traffic!
			if (!productArchetypeCombinable.contains(craftedItemArchetypeIdCount.first))
			{
				for (uint count = 0; count < craftedItemArchetypeIdCount.second; count++)
					HkAddCargo(ARG_CLIENTID(clientId), craftedItemArchetypeIdCount.first, 1, false);
			}
			// Stackable items will be sent as a batch to reduce package traffic.
			else
			{
				HkAddCargo(ARG_CLIENTID(clientId), craftedItemArchetypeIdCount.first, craftedItemArchetypeIdCount.second, false);
			}
		}

		std::wstring message = L"Obtained";
		if (shipGoodId != 0)
		{
			const GoodInfo* shipGood = GoodList::find_by_id(shipGoodId);
			if (shipGood)
			{
				const GoodInfo* hullGood = GoodList::find_by_id(shipGood->iHullGoodID);
				if (hullGood)
				{
					const auto shipArch = Archetype::GetShip(hullGood->shipArchId);
					if (shipArch)
						message += L" a " + HkGetWStringFromIDS(shipArch->iIdsName);
				}
			}
		}
		else if (producedArchetypeIds.size() > 5)
		{
			message += L" many items";
		}
		else
		{
			size_t count = 1;
			for (const auto& producedItemArchetypeIdCount : producedArchetypeIds)
			{
				message += L" " + std::to_wstring(producedItemArchetypeIdCount.second) + L" '" + productNamesByArchetypeId.at(producedItemArchetypeIdCount.first) + L"'";
				if (count < producedArchetypeIds.size())
					message += L",";
				count++;
			}
		}

		message += L" from " + std::to_wstring(batchCount) + L" '" + recipe.originalName + L"'";
		if (recipe.cost)
			message += L" for " + PrintMoney(recipe.cost * batchCount);
		PrintUserCmdText(clientId, message + L".");

		const uint soundId = recipe.successSoundId ? recipe.successSoundId : defaultSuccessSoundId;
		if (soundId)
			pub::Audio::PlaySoundEffect(clientId, soundId);
		pub::Player::SendNNMessage(clientId, shipGoodId ? ALL_SYSTEMS_NORMAL_ID : LOADED_INTO_CARGO_HOLD_ID);

		return true;
	}

	static bool Craft(const uint clientId, const std::string& recipeName, const RecipeType type, int batchCount)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return false;

		const Recipe* recipe = FindRecipe(clientId, recipeName);
		if (!recipe)
			return false;

		if (type != RecipeType::Any && recipe->type != type)
		{
			PrintUserCmdText(clientId, L"Scheme '" + stows(recipeName) + L"' cannot be used for this!");
			return false;
		}

		batchCount = CorrectBatchCount(clientId, *recipe, batchCount);
		if (batchCount == 0)
			return false;

		bool result = TestAllRequirements(clientId, *recipe, batchCount);
		if (result)
		{
			if (recipe->shipsCount > 0)
			{
				if (const auto& entry = currentShipCraftingPopups.find(clientId); entry != currentShipCraftingPopups.end() && entry->second == recipe->originalName)
				{
					currentShipCraftingPopups.erase(entry);
					result = ConsumeAllIngredients(clientId, *recipe, batchCount) && ProduceItems(clientId, *recipe, batchCount);
				}
				else
				{
					currentShipCraftingPopups.erase(clientId);
					currentShipCraftingPopups.insert({ clientId, recipe->originalName });
					const bool allShips = recipe->shipsCount == recipe->products.size();
					pub::Player::PopUpDialog(clientId, FmtStr(allShips ? 524393 : 524395, 0), FmtStr(allShips ? 524394 : 524396, 0), PopupDialogButton::LEFT_YES | PopupDialogButton::RIGHT_LATER);
				}
			}
			else
			{
				result = ConsumeAllIngredients(clientId, *recipe, batchCount) && ProduceItems(clientId, *recipe, batchCount);
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
		bool craft = ToLower(argumentsWS).find(L"/craft") == 0;
		bool loot = ToLower(argumentsWS).find(L"/loot") == 0;
		bool dismantle = ToLower(argumentsWS).find(L"/dismantle") == 0;
		if (craft || loot || dismantle)
		{
			const std::wstring& arguments = Trim(GetParamToEnd(argumentsWS, ' ', 1));
			const size_t lastWhiteSpace = arguments.find_last_of(' ');
			int count = ToInt(arguments.substr(lastWhiteSpace + 1));
			std::wstring recipeName = arguments;
			if (count != 0)
				recipeName = Trim(arguments.substr(0, lastWhiteSpace));
			currentShipCraftingPopups.erase(clientId); // Make sure to not skip the popup this way.
			RecipeType type;
			if (craft)
				type = RecipeType::Produce;
			else if (loot)
				type = RecipeType::Loot;
			else if (dismantle)
				type = RecipeType::Dismantle;
			Craft(clientId, ToLower(wstos(recipeName)), type, std::min<int>(1000, std::max<int>(1, count)));
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
				Craft(clientId, ToLower(wstos(entry->second)), RecipeType::Any, 1);
			else
				currentShipCraftingPopups.erase(entry);
			returncode = SKIPPLUGINS;
			return;
		}
		returncode = DEFAULT_RETURNCODE;
	}
}