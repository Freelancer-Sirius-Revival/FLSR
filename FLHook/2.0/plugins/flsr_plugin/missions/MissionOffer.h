#pragma once
#include <FLHook.h>

namespace Missions
{
	enum class MissionReofferCondition
	{
		Always,
		OnFail,
		OnSuccess,
		Never
	};

	struct MissionOffer
	{
		pub::GF::MissionType type = pub::GF::MissionType::Unknown;
		uint system = 0;
		uint group = 0;
		uint title = 0;
		uint description = 0;
		uint reward = 0;
		std::vector<uint> bases;
		MissionReofferCondition reofferCondition = MissionReofferCondition::Never;
		int reofferDelay = 0;
	};
}