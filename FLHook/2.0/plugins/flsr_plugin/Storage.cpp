#include "Main.h"
#include "rpcdce.h"
#include <regex>

namespace Storage
{
	struct Item
	{
		uint archetypeId;
		uint count = 1;
	};

	struct Storage
	{
		uint baseId;
		std::unordered_map<uint, uint> itemArchetypeIdsWithCount = {};
		//std::unordered_map<Archetype::AClassType, std::vector<Item>> itemsByType;
	};

	struct Account
	{
		std::string uid;
		std::wstring ownerAccountId;
		int64_t money = 0;
		std::unordered_map<uint, Storage> storagesByBaseId = {};
	};

	static std::unordered_map<std::string, Account> accountByAccountUid;
	static std::vector<uint> excludedBaseIds;
	static std::unordered_map<std::string, std::string> accountUidByCharacterFileName;

	static std::unordered_map<uint, std::unordered_map<uint, uint>> itemArchetypeIdByChatIdPerClientId;

	static std::string outputDirectory;
	static int maxCharacterMoney = 999999999;

	static std::unordered_map<std::wstring, uint> baseIdsByLoweredDisplayName;

	std::wstring GetBaseName(const uint baseId)
	{
		uint strId;
		pub::GetBaseStridName(strId, baseId);
		if (strId)
			return HkGetWStringFromIDS(strId);
		return std::to_wstring(baseId);
	}

	std::wstring GetEquipmentName(const uint archetypeId)
	{
		const GoodInfo* goodInfo = GoodList::find_by_id(archetypeId);
		if (goodInfo && goodInfo->iIDSName > 0)
			return HkGetWStringFromIDS(goodInfo->iIDSName);
		return std::to_wstring(archetypeId);
	}

	void LoadStorage(const std::wstring& filePath)
	{
		INI_Reader ini;
		if (ini.open(wstos(filePath).c_str(), false))
		{
			Account account;
			account.uid = "";
			while (ini.read_header())
			{
				if (ini.is_header("Meta"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("account_uid"))
							account.uid = ini.get_value_string(0);
						if (ini.is_value("owner_account"))
							account.ownerAccountId = stows(ini.get_value_string(0));
					}
				}
				else if (ini.is_header("Global"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("money"))
						{
							account.money = 0;
							const int64_t value = strtoll(ini.get_value_string(0), NULL, 0);
							if (value > 0 && value < LLONG_MAX)
								account.money = value;
						}
					}
				}
				else
				{
					uint baseId;
					pub::GetBaseID(baseId, ini.get_header_ptr());
					if (baseId)
					{
						Storage storage;
						storage.baseId = baseId;
						while (ini.read_value())
						{
							const std::string line = std::string(ini.get_line_ptr());
							const std::string key = Trim(line.substr(0, line.find("=")));
							const uint itemArchetypeId = strtoul(key.c_str(), NULL, 0);
							if (itemArchetypeId != 0)
								storage.itemArchetypeIdsWithCount[itemArchetypeId] = ini.get_value_int(0);
						}
						account.storagesByBaseId[baseId] = storage;
					}
				}
			}
			ini.close();

			if (account.uid.empty())
				return;

			accountByAccountUid[account.uid] = account;
		}
	}

	void LoadLinkedCharacters(const std::wstring& filePath)
	{
		INI_Reader ini;
		if (ini.open(wstos(filePath).c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("LinkedCharacters"))
				{
					while (ini.read_value())
					{
						const std::string line = std::string(ini.get_line_ptr());
						const std::string key = Trim(line.substr(0, line.find("=")));
						accountUidByCharacterFileName[key] = ini.get_value_string(0);
					}
				}
			}
			ini.close();
		}
	}

	static bool initialized = false;
	void InitializeStorageSystem()
	{
		if (initialized)
			return;
		initialized = true;

		// Set up the directory name for all save files of this plugin.
		outputDirectory = scAcctPath + "\\storages\\";

		if (!std::filesystem::is_directory(outputDirectory))
			return;

		// Read all storage save files
		const std::regex filePattern(".{4}-.{4}-.{4}-.{4}\\.ini", std::regex_constants::ECMAScript | std::regex_constants::icase);
		for (const auto& entry : std::filesystem::directory_iterator(outputDirectory))
		{
			if (std::regex_match(wstos(entry.path().filename()), filePattern))
				LoadStorage(entry.path());
			else if (entry.path().filename() == L"linked_chars.ini")
				LoadLinkedCharacters(entry.path());
		}

		HkLoadStringDLLs();

		// Find the actually set up character money limit.
		const HMODULE serverHandle = GetModuleHandle("server.dll");
		if (serverHandle)
			maxCharacterMoney = *(int*)(DWORD(serverHandle) + 0x06F46E);

		// Cache all relevant base display names as lowered strings together with their IDs for the /stored command
		Universe::IBase* base = Universe::GetFirstBase();
		while (base)
		{
			const std::wstring& baseNickname = ToLower(HkGetBaseNickByID(base->iBaseID));
			if (!baseNickname.starts_with(L"intro"))
			{
				const std::wstring& name = ToLower(GetBaseName(base->iBaseID));
				if (!name.empty())
					baseIdsByLoweredDisplayName[name] = base->iBaseID;
			}
			base = Universe::GetNextBase();
		}
	}

	std::string GetCharacterFileName(const uint clientId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return "";
		std::wstring characterFileName;
		if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileName) != HKE_OK)
			return "";
		return wstos(characterFileName);
	}

	void SwitchToAccount(const uint clientId, const std::string accountUid)
	{
		if (!accountByAccountUid.contains(accountUid))
		{
			PrintUserCmdText(clientId, L"Unable to switch to storage account. No account exists for the given ID.");
			return;
		}

		const std::string characterFileName = GetCharacterFileName(clientId);
		if (characterFileName.empty())
		{
			PrintUserCmdText(clientId, L"Failed finding your character.");
			return;
		}

		IniWrite(outputDirectory + "\\linked_chars.ini", "LinkedCharacters", characterFileName, accountUid);
		accountUidByCharacterFileName[characterFileName] = accountUid;
		PrintUserCmdText(clientId, L"Switched to the storage account!");
	}

	bool HasAccount(const uint clientId)
	{
		return accountUidByCharacterFileName.contains(GetCharacterFileName(clientId));
	}

	bool IsPlayerDocked(const uint clientId)
	{
		uint baseId;
		pub::Player::GetBase(clientId, baseId);
		return baseId;
	}

	Account& GetAccount(const uint clientId)
	{
		const std::string& accountUid = accountUidByCharacterFileName[GetCharacterFileName(clientId)];
		return accountByAccountUid[accountUid];
	}

	void ShowCurrentAccountUid(const uint clientId)
	{
		if (HasAccount(clientId))
			PrintUserCmdText(clientId, L"Active storage account ID: " + stows(GetAccount(clientId).uid));
		else
			PrintUserCmdText(clientId, L"No active storage account. Create one by typing: /storage new");
	}

	void CreateNewAccount(const uint clientId)
	{
		uint attempts = 0;
		std::string uuidString = "";
		do
		{
			UUID uuid;
			if (UuidCreate(&uuid) != RPC_S_OK)
				return;
			RPC_CSTR uuidStringOutput;
			if (UuidToStringA(&uuid, &uuidStringOutput) != RPC_S_OK)
				return;
			// Keep the first four 4-digit blocks as actual ID to have a big enough number of possibilities to prevent brute force attacks
			uuidString = std::string((char*)uuidStringOutput).substr(9, 19);
			RpcStringFreeA(&uuidStringOutput);
		}
		while (accountByAccountUid.contains(uuidString) && attempts++ < 10);

		if (attempts > 9)
		{
			PrintUserCmdText(clientId, L"Failed to create new storage account. Try again.");
			return;
		}
		
		if (CreateDirectoryA(outputDirectory.c_str(), NULL) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			PrintUserCmdText(clientId, L"Failed to create storage account database directory. Report to an admin!");
			return;
		}
		
		const std::wstring clientAccountId = HkGetAccountIDByClientID(clientId);
		IniWrite(outputDirectory + "\\" + uuidString + ".ini", "Meta", "account_uid", uuidString);
		IniWrite(outputDirectory + "\\" + uuidString + ".ini", "Meta", "owner_account", wstos(clientAccountId));

		accountByAccountUid[uuidString].ownerAccountId = clientAccountId;
		accountByAccountUid[uuidString].uid = uuidString;
		
		PrintUserCmdText(clientId, L"Created storage account by ID: " + stows(uuidString) + L" Save it and keep it secret!");

		SwitchToAccount(clientId, uuidString);
	}

	void ListStorages(const uint clientId)
	{
		if (!HasAccount(clientId))
		{
			PrintUserCmdText(clientId, L"Without active storage account you cannot list places with stored items!");
			return;
		}

		std::wstring result = L"";
		const Account& account = GetAccount(clientId);
		for (const auto& storageByBaseId : account.storagesByBaseId)
		{
			if (!storageByBaseId.second.itemArchetypeIdsWithCount.empty())
				result = result + (result.empty() ? L"" : L", ") + GetBaseName(storageByBaseId.first);
		}

		if (result.empty())
			PrintUserCmdText(clientId, L"You have no stored items anywhere.");
		else
			PrintUserCmdText(clientId, L"Stored items on: " + result);
	}

	uint FindBaseIdByDisplayName(const std::wstring baseDisplayName)
	{
		const auto& foundBaseId = baseIdsByLoweredDisplayName.find(ToLower(baseDisplayName));
		if (foundBaseId != baseIdsByLoweredDisplayName.end())
			return foundBaseId->second;
		return 0;
	}

	void ListStoredItems(const uint clientId, const uint baseId)
	{
		if (!HasAccount(clientId))
		{
			PrintUserCmdText(clientId, L"Without active storage account you cannot list stored items!");
			return;
		}

		const Account& account = GetAccount(clientId);
		if (!account.storagesByBaseId.contains(baseId) || account.storagesByBaseId.at(baseId).itemArchetypeIdsWithCount.empty())
		{
			PrintUserCmdText(clientId, L"You have no items stored on " + GetBaseName(baseId) + L".");
			return;
		}

		const Storage& storage = account.storagesByBaseId.at(baseId);
		std::unordered_map<std::wstring, uint> itemCountByName;
		std::set<std::wstring> itemNames;
		for (const auto& itemIdWithCount : storage.itemArchetypeIdsWithCount)
		{
			const std::wstring name = GetEquipmentName(itemIdWithCount.first);
			itemNames.insert(name);
			itemCountByName[name] = itemIdWithCount.second;
		}

		uint number = 1;
		for (const std::wstring& name : itemNames)
			PrintUserCmdText(clientId, L"[" + std::to_wstring(number++) + L"] " + std::to_wstring(itemCountByName[name]) + L"\u00D7 " + name);
	}

	void ListUnmountedCharacterItems(const uint clientId)
	{
		if (!IsPlayerDocked(clientId))
		{
			PrintUserCmdText(clientId, L"You must be docked to list your cargo hold!");
			return;
		}

		int remainingHoldSize;
		std::list<CARGO_INFO> cargoList;
		if (HkEnumCargo(ARG_CLIENTID(clientId), cargoList, remainingHoldSize) != HKE_OK)
			return;
		std::unordered_map<std::wstring, std::pair<uint, uint>> itemCountAndArchetypeIdByName;
		std::set<std::wstring> itemNames;
		for (const CARGO_INFO& cargo : cargoList)
		{
			if (!cargo.bMounted && !cargo.bMission)
			{
				const std::wstring name = GetEquipmentName(cargo.iArchID);
				itemNames.insert(name);
				itemCountAndArchetypeIdByName[name] = {
					(!itemCountAndArchetypeIdByName.contains(name) ? 0 : itemCountAndArchetypeIdByName[name].first) + cargo.iCount,
					cargo.iArchID
				};
			}
		}

		uint number = 1;
		for (const std::wstring& name : itemNames)
		{
			itemArchetypeIdByChatIdPerClientId[clientId][number] = itemCountAndArchetypeIdByName[name].second;
			PrintUserCmdText(clientId, L"[" + std::to_wstring(number++) + L"] " + std::to_wstring(itemCountAndArchetypeIdByName[name].first) + L"\u00D7 " + name);
		}
	}

	void StoreItem(const uint clientId, const uint baseId, const uint itemArchetypeId, const uint amount)
	{
		if (!IsPlayerDocked(clientId))
		{
			PrintUserCmdText(clientId, L"You must be docked to store items!");
			return;
		}

		if (!HasAccount(clientId))
		{
			PrintUserCmdText(clientId, L"Cannot store an item without active storage account!");
			return;
		}

		int remainingHoldSize;
		std::list<CARGO_INFO> cargoList;
		if (HkEnumCargo(ARG_CLIENTID(clientId), cargoList, remainingHoldSize) != HKE_OK)
			return;
		std::unordered_map<uint, uint> itemIdsWithCount;
		int remainingAmount = amount;
		for (const auto& foo : cargoList)
		{
			if (!foo.bMission && !foo.bMounted && foo.iArchID == itemArchetypeId)
			{
				const int amountToRemove = std::min(remainingAmount, foo.iCount);
				itemIdsWithCount[foo.iID] = amountToRemove;
				remainingAmount -= amountToRemove;
				if (remainingAmount <= 0)
					break;
			}
		}
		if (remainingAmount > 0)
		{
			PrintUserCmdText(clientId, L"Cannot store more items from your cargo than you have!");
			return;
		}

		for (const auto& itemIdWithCount : itemIdsWithCount)
			pub::Player::RemoveCargo(clientId, itemIdWithCount.first, itemIdWithCount.second);

		Account& account = GetAccount(clientId);
		auto& itemArchetypeIdsWithCount = account.storagesByBaseId[baseId].itemArchetypeIdsWithCount;
		if (!itemArchetypeIdsWithCount.contains(itemArchetypeId))
			itemArchetypeIdsWithCount[itemArchetypeId] = 0;
		itemArchetypeIdsWithCount[itemArchetypeId] += amount;

		const std::wstring baseNickname = HkGetBaseNickByID(baseId);
		IniWrite(outputDirectory + "\\" + account.uid + ".ini", wstos(baseNickname), std::to_string(itemArchetypeId), std::to_string(itemArchetypeIdsWithCount[itemArchetypeId]));
	}

	void UnstoreItem(const uint clientId, const uint baseId, const uint itemId)
	{

	}

	std::wstring PrintMoney(const int64_t amount)
	{
		std::wstring result = std::to_wstring(amount);
		for (int pos = result.size() - 3; pos > 0; pos = pos - 3)
			result = result.insert(pos, L",");
		return L"$" + result;
	}

	void DepositMoney(const uint clientId, const int64_t amount)
	{
		if (!IsPlayerDocked(clientId))
		{
			PrintUserCmdText(clientId, L"You must be docked to be able to deposit money!");
			return;
		}

		if (!HasAccount(clientId))
		{
			PrintUserCmdText(clientId, L"Cannot deposit money without active storage account!");
			return;
		}

		if (amount < 0)
		{
			PrintUserCmdText(clientId, L"Please enter a bigger sum than 0 to deposit!");
			return;
		}

		int currentCash = 0;
		if (HkGetCash(ARG_CLIENTID(clientId), currentCash) != HKE_OK || currentCash < amount)
		{
			PrintUserCmdText(clientId, L"Cannot deposit more money than you own!");
			return;
		}
		
		if (HkAddCash(ARG_CLIENTID(clientId), static_cast<int>(-amount)) != HKE_OK)
		{
			PrintUserCmdText(clientId, L"Failed to deposit cash. Try again.");
			return;
		}

		Account& account = GetAccount(clientId);
		account.money += amount;
		IniWrite(outputDirectory + "\\" + account.uid + ".ini", "Global", "money", std::to_string(account.money));
		PrintUserCmdText(clientId, PrintMoney(amount) + L" deposited to bank.");
	}

	void WithdrawMoney(const uint clientId, const int64_t amount)
	{
		if (!IsPlayerDocked(clientId))
		{
			PrintUserCmdText(clientId, L"You must be docked to be able to withdraw money!");
			return;
		}

		if (!HasAccount(clientId))
		{
			PrintUserCmdText(clientId, L"Cannot withdraw money without active storage account!");
			return;
		}

		Account& account = GetAccount(clientId);
		if (account.money < amount)
		{
			PrintUserCmdText(clientId, L"Cannot withdraw more money than you own! Current banked money: " + PrintMoney(account.money) + L".");
			return;
		}

		// Limit the money to never exceed whatever is currently set up in server.dll
		int currentCash = INT_MAX;
		if (HkGetCash(ARG_CLIENTID(clientId), currentCash) != HKE_OK || currentCash + amount > maxCharacterMoney)
		{
			PrintUserCmdText(clientId, L"Your character's money cannot exceed " + PrintMoney(maxCharacterMoney) + L".");
			return;
		}

		if (HkAddCash(ARG_CLIENTID(clientId), static_cast<int>(amount)) != HKE_OK)
		{
			PrintUserCmdText(clientId, L"Failed to withdraw cash. Try again.");
			return;
		}

		account.money -= amount;
		IniWrite(outputDirectory + "\\" + account.uid + ".ini", "Global", "money", std::to_string(account.money));
		PrintUserCmdText(clientId, PrintMoney(amount) + L" withdrawn from bank.");
	}

	void ShowCurrentMoney(const uint clientId)
	{
		if (!HasAccount(clientId))
		{
			PrintUserCmdText(clientId, L"Cannot show banked money without active storage account!");
			return;
		}

		const Account& account = GetAccount(clientId);
		PrintUserCmdText(clientId, L"Currently banked money: " + PrintMoney(account.money) + L".");
	}

	bool UserCmd_Storage(const uint clientId, const std::wstring& arguments)
	{
		returncode = DEFAULT_RETURNCODE;
		const std::wstring argumentsLowered = ToLower(arguments);

		if (argumentsLowered.starts_with(L"/deposit"))
		{
			const std::string& arguments = Trim(wstos(GetParamToEnd(argumentsLowered, ' ', 1)));
			const int64_t value = strtoll(arguments.c_str(), NULL, 0);
			if (value > 0 && value != LLONG_MAX)
			{
				DepositMoney(clientId, value);
			}
			else
			{
				PrintUserCmdText(clientId, L"Please enter a proper amount of money to deposit.");
			}

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		if (argumentsLowered.starts_with(L"/withdraw"))
		{
			const std::string& arguments = Trim(wstos(GetParamToEnd(argumentsLowered, ' ', 1)));
			const int64_t value = strtoll(arguments.c_str(), NULL, 0);
			if (value > 0 && value != LLONG_MAX)
			{
				WithdrawMoney(clientId, value);
			}
			else
			{
				PrintUserCmdText(clientId, L"Please enter a proper amount of money to withdraw.");
			}

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		if (argumentsLowered.starts_with(L"/money") || argumentsLowered.starts_with(L"/bank"))
		{
			ShowCurrentMoney(clientId);
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		if (argumentsLowered.starts_with(L"/inventory"))
		{
			ListUnmountedCharacterItems(clientId);
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		if (argumentsLowered.starts_with(L"/store"))
		{
			uint baseId;
			pub::Player::GetBase(clientId, baseId);

			const std::wstring& argument1 = Trim(GetParam(argumentsLowered, ' ', 1));
			const uint itemNumber = strtoul(wstos(argument1).c_str(), NULL, 0);
			const std::wstring& argument2 = Trim(GetParam(argumentsLowered, ' ', 2));
			const uint itemAmount = strtoul(wstos(argument2).c_str(), NULL, 0);
			if (itemNumber > 0 && itemArchetypeIdByChatIdPerClientId[clientId].contains(itemNumber))
				StoreItem(clientId, baseId, itemArchetypeIdByChatIdPerClientId[clientId][itemNumber], std::max(static_cast<uint>(1), (itemAmount == ULONG_MAX ? 0 : itemAmount)));
		}

		if (argumentsLowered.starts_with(L"/storages"))
		{
			ListStorages(clientId);
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		if (argumentsLowered.starts_with(L"/stored"))
		{
			const std::wstring& arguments = Trim(GetParamToEnd(argumentsLowered, ' ', 1));
			uint baseId = 0;
			if (!arguments.empty())
				baseId = FindBaseIdByDisplayName(arguments);
			else
				pub::Player::GetBase(clientId, baseId);

			if (baseId > 0)
				ListStoredItems(clientId, baseId);
			else
				PrintUserCmdText(clientId, L"Specify the base to list stored items from! /stored <base name>");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		if (argumentsLowered.starts_with(L"/storage"))
		{
			const std::string& arguments = Trim(wstos(GetParamToEnd(argumentsLowered, ' ', 1)));
			if (arguments == "new")
			{
				CreateNewAccount(clientId);
			}
			else if (arguments.length() == 0)
			{
				ShowCurrentAccountUid(clientId);
			}
			else
			{
				SwitchToAccount(clientId, arguments);
			}

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		return false;
	}
}
