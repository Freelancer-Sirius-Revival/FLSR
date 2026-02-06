#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	extern std::unordered_map<uint, Costume> costumeById;

	void ReadCostumeData();
}