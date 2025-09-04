#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	extern std::unordered_map<byte, std::vector<float>> npcWaveSizeByDifficulty;

	void ReadNpcWaveData();
}