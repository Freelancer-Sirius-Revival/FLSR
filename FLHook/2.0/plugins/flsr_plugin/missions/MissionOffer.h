#pragma once
#include <FLHook.h>

namespace Missions
{
	struct MissionOffer
	{
		pub::GF::MissionType type = pub::GF::MissionType::Unknown;
		uint system = 0;
		uint group = 0;
		uint text = 0;
		uint reward = 0;
		std::vector<uint> bases;
	};
}