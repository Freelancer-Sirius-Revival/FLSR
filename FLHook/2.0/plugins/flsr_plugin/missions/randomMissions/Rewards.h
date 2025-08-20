#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	extern std::unordered_map<float, uint> moneyByDifficulty;

	void ReadRewardData();
}