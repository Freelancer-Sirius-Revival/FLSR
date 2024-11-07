#include "Main.h"
#include "rpcdce.h"

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
		std::unordered_map<Archetype::AClassType, std::vector<Item>> itemsByType;
	};

	struct Account
	{
		std::string uid;
		uint64_t money = 0;
		std::unordered_map<uint, Storage> storages;
	};

	static std::unordered_map<std::string, Account> accounts;
	static std::unordered_map<uint, std::string> itemArchetypeIdDisplayNames;

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
			uuidString = std::string((char*)uuidStringOutput).substr(9, 14);
			RpcStringFreeA(&uuidStringOutput);
		}
		while (accounts.contains(uuidString) && attempts++ < 10);

		if (attempts > 9)
		{
			PrintUserCmdText(clientId, L"Failed to create new account. Try again.");
			return;
		}
		
		const std::string outputDirectory = scAcctPath + "\\storages\\";
		if (CreateDirectoryA(outputDirectory.c_str(), NULL) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			PrintUserCmdText(clientId, L"Failed to create account database directory. Report to an admin!");
			return;
		}
			
		HANDLE fileHandle = CreateFileA((outputDirectory + "\\" + uuidString + ".ini").c_str(), NULL, NULL, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			PrintUserCmdText(clientId, L"Failed to create account database file. Report to an admin!");
			return;
		}
		CloseHandle(fileHandle);

		accounts[uuidString].uid = uuidString;

		PrintUserCmdText(clientId, L"Created account ID: " + stows(uuidString) + L" Note it down and keep it secret!");
	}

	void SwitchToAccount()
	{

	}

	void ListStoredItems()
	{

	}

	void ListCharacterItems()
	{

	}

	void StoreItem()
	{

	}

	void UnstoreItem()
	{

	}

	void DepositMoney()
	{

	}

	void WithdrawMoney()
	{

	}

	void ListStorages()
	{

	}

	void UserCmd_CreateAccount(const uint clientId, const std::wstring& arguments)
	{
		CreateNewAccount(clientId);
	}
}
