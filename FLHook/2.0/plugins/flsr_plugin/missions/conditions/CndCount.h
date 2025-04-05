#pragma once
#include "Condition.h"
#include "CndCountArch.h"

namespace Missions
{
	struct CndCount : Condition
	{
		CndCountArchetypePtr archetype;

		CndCount(const ConditionParent& parent, const CndCountArchetypePtr conditionArchetype);
		~CndCount();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint baseId);
	};

	extern std::unordered_map<uint, std::unordered_set<CndCount*>> countConditionsByMission;
}