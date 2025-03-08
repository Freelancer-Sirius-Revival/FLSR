#pragma once
#include <utility>

namespace NpcNames
{
	std::pair<unsigned int, unsigned int> GetRandomName(const unsigned int factionId, const unsigned int voiceId);
	std::pair<unsigned int, unsigned int> GetRandomLargeShipName(const unsigned int factionId);
	void ReadFiles();
}