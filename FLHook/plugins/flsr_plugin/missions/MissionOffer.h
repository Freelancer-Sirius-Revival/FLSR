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
		std::unordered_set<uint> shipArchetypeIds;
		std::unordered_set<uint> baseIds;
		MissionReofferCondition reofferCondition = MissionReofferCondition::Never;
		float reofferDelay = 0.0f;
	};
}