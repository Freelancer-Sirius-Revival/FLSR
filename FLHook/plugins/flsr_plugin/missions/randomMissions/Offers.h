#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	struct BaseOffer
	{
		uint baseId;
		uint factionId = 0;
		float weight = 0.0f;
		float minDifficulty = 0.0f;
		float maxDifficulty = 0.0f;
	};

	extern std::unordered_map<uint, uint> owningFactionByBaseId;
	extern std::unordered_map<uint, std::vector<BaseOffer>> offersByBaseId;

	void ReadOfferData();
}