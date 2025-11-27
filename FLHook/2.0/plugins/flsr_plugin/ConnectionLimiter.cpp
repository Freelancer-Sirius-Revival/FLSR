#include "Main.h"

namespace ConnectionLimiter
{
	uint maxParallelConnectionsPerIpAddress = 2;

	static std::unordered_map<uint, std::wstring> ipAddressByClientId;
	static std::unordered_map<std::wstring, ushort> ipAddressesCounter;

	static bool canHaveUnlimitedCharacters(const uint clientId)
	{
		CAccount* account = Players.FindAccountFromClientID(clientId);
		if (!account)
			return false;

		std::wstring accountDirectory;
		HkGetAccountDirName(account, accountDirectory);
		const std::string adminFile = scAcctPath + wstos(accountDirectory) + "\\flhookadmin.ini";

		INI_Reader ini;
		if (ini.open(adminFile.c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("admin"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("rights") && ToLower(std::string(ini.get_value_ptr())).find("superadmin") != -1)
						{
							return true;
						}
					}
				}
			}
			ini.close();
		}
		return false;
	}

	static void kickPlayer(const uint clientId)
	{
		if (clientId > 0 && clientId <= MAX_CLIENT_ID)
		{
			CDPClientProxy* cdpClient = g_cClientProxyArray[clientId - 1];
			if (cdpClient)
				cdpClient->Disconnect();
		}
	}

	void __stdcall Login_After(struct SLoginInfo const& li, unsigned int clientId)
	{
		std::wstring ipAddress;
		HkGetPlayerIP(clientId, ipAddress);
		if (ipAddress == L"")
		{
			kickPlayer(clientId);
			returncode = DEFAULT_RETURNCODE;
			return;
		}
		
		if (canHaveUnlimitedCharacters(clientId))
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		ipAddressByClientId[clientId] = ipAddress;
		ipAddressesCounter[ipAddress]++;
		if (ipAddressesCounter[ipAddress] > maxParallelConnectionsPerIpAddress)
			kickPlayer(clientId);

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2)
	{
		std::wstring ipAddress = ipAddressByClientId[clientId];
		if (ipAddress != L"")
			ipAddressesCounter[ipAddress] = static_cast<ushort>(std::max(ipAddressesCounter[ipAddress] - 1, 0));
		ipAddressByClientId.erase(clientId);
		returncode = DEFAULT_RETURNCODE;
	}
}
