#include "MissionAbortFix.h"

namespace MissionAbortFix
{
	std::unordered_map<uint, CHARACTER_ID> lastCharacterByClientId;

	void __stdcall CharacterSelect(const CHARACTER_ID& cId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		const auto& foundEntry = lastCharacterByClientId.find(clientId);
		if (foundEntry == lastCharacterByClientId.end() || !(foundEntry->second == cId))
		{
			uint missionId;
			pub::Player::GetMsnID(clientId, missionId);
			if (missionId > 0)
			{
				Server.AbortMission(clientId, missionId);
				// AbortMission cleans assigned mission IDs up. But if we have an invalid or custom-assigned ID, it will not do anything.
				pub::Player::SetMsnID(clientId, 0, 0, false, 0);
			}
		}
	}

	void __stdcall CharacterSelect_AFTER(const CHARACTER_ID& cId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		lastCharacterByClientId[clientId] = cId;
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2)
	{
		returncode = DEFAULT_RETURNCODE;
		uint missionId;
		pub::Player::GetMsnID(clientId, missionId);
		if (missionId > 0)
		{
			Server.AbortMission(clientId, missionId);
			// AbortMission cleans assigned mission IDs up. But if we have an invalid or custom-assigned ID, it will not do anything.
			pub::Player::SetMsnID(clientId, 0, 0, false, 0);
		}
	}
}