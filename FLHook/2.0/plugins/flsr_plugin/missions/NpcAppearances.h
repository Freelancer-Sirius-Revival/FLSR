#pragma once
#include <FLHook.h>

namespace NpcAppearances
{
	std::pair<uint, uint> GetRandomName(const uint factionId, const uint voiceId);
	std::pair<uint, uint> GetRandomLargeShipName(const uint factionId);
	Costume GetRandomCostume(const uint factionId, const uint voiceId);
	void ReadFiles();
}