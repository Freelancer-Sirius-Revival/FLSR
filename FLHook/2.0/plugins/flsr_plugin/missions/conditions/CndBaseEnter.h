#pragma once
#include "Condition.h"
#include "CndBaseEnterArch.h"

namespace Missions
{
	struct CndBaseEnter : Condition
	{
		CndBaseEnterArchetypePtr archetype;

		CndBaseEnter(const ConditionParent& parent, const CndBaseEnterArchetypePtr conditionArchetype);
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint baseId);
	};

	extern std::unordered_set<CndBaseEnter*> baseEnterConditions;
}