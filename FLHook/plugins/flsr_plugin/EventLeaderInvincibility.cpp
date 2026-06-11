#include "Main.h"
#include "EventLeaderInvincibility.h"

namespace EventLeaderInvincibility
{
	uint leaderClientId = 0;
	std::unordered_set<uint> escortClientIds;
	float leaderProtectionDistance = 5000.0f;

	void __stdcall Elapse_Time_AFTER(float seconds)
	{
		if (!leaderClientId)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		uint leaderShipId = 0;
		pub::Player::GetShip(leaderClientId, leaderShipId);
		if (!leaderShipId)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		bool escortsNearby = false;
		uint leaderSystemId = 0;
		pub::Player::GetSystem(leaderClientId, leaderSystemId);
		for (const uint escortClientId : escortClientIds)
		{
			uint escortShipId = 0;
			pub::Player::GetShip(escortClientId, escortShipId);
			if (!escortShipId)
				continue;

			uint escortSystemId = 0;
			pub::Player::GetSystem(escortClientId, escortSystemId);
			if (escortSystemId != leaderSystemId)
				continue;

			if (HkDistance3DByShip(leaderShipId, escortShipId) <= leaderProtectionDistance)
			{
				escortsNearby = true;
				break;
			}
		}

		if (escortsNearby)
			pub::SpaceObj::SetInvincible2(leaderShipId, true, true, 0);
		else
			pub::SpaceObj::SetInvincible2(leaderShipId, false, false, 0);

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state)
	{
		if (leaderClientId == clientId)
			leaderClientId = 0;
		escortClientIds.erase(clientId);

		returncode = DEFAULT_RETURNCODE;
	}

	bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd)
	{
		if (IS_CMD("distance"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const auto value = strtol(wstos(ToLower(cmds->ArgStr(1))).c_str(), NULL, 0);
			if (value != 0 && value != LONG_MAX && value != LONG_MIN)
			{
				leaderProtectionDistance = std::max<float>(0.0f, value);
				PrintUserCmdText(clientId, L"New event leader invincibility range: " + std::to_wstring(leaderProtectionDistance));
			}
			else
				PrintUserCmdText(clientId, L"Please enter a proper number.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		else if (IS_CMD("leader"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			if (escortClientIds.contains(clientId))
			{
				PrintUserCmdText(clientId, L"You already are event escort!");
			}
			else if (leaderClientId == clientId)
			{
				leaderClientId = 0;
				PrintUserCmdText(clientId, L"You aren't event leader anymore.");
				for (const uint escortClientId : escortClientIds)
					PrintUserCmdText(escortClientId, L"ID " + std::to_wstring(clientId) + L" is no more event leader.");
			}
			else
			{
				if (leaderClientId)
					PrintUserCmdText(leaderClientId, L"ID " + std::to_wstring(clientId) + L" is now event leader.");
				for (const uint escortClientId : escortClientIds)
					PrintUserCmdText(escortClientId, L"ID " + std::to_wstring(clientId) + L" is now event leader.");
				leaderClientId = clientId;
				PrintUserCmdText(clientId, L"You are now event leader.");
			}

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		else if (IS_CMD("escort"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			if (leaderClientId == clientId)
			{
				PrintUserCmdText(clientId, L"You already are event leader!");
			}
			else if (escortClientIds.contains(clientId))
			{
				escortClientIds.erase(clientId);
				PrintUserCmdText(clientId, L"You aren't event escort anymore.");
				if (leaderClientId)
					PrintUserCmdText(leaderClientId, L"ID " + std::to_wstring(clientId) + L" left event escorts.");
				for (const uint escortClientId : escortClientIds)
					PrintUserCmdText(escortClientId, L"ID " + std::to_wstring(clientId) + L" left event escorts.");
			}
			else
			{
				if (leaderClientId)
					PrintUserCmdText(leaderClientId, L"ID " + std::to_wstring(clientId) + L" joined event escorts.");
				for (const uint escortClientId : escortClientIds)
					PrintUserCmdText(escortClientId, L"ID " + std::to_wstring(clientId) + L" joined event escorts.");
				escortClientIds.insert(clientId);
				PrintUserCmdText(clientId, L"You are now event escort.");
			}

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		returncode = DEFAULT_RETURNCODE;
		return false;
	}
}