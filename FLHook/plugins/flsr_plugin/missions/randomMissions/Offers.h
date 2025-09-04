#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	struct BaseOffer
	{
		uint baseId;
		float minDifficulty;
		float maxDifficulty;
		uint factionId;
		float weight;
	};

	extern std::unordered_map<uint, std::vector<BaseOffer>> offersByBaseId;
	extern std::unordered_map<uint, std::pair<byte, byte>> offerCountByBaseId;

	void ReadOfferData();
}