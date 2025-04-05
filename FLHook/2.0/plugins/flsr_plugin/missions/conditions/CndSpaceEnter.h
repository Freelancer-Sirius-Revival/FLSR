#pragma once
#include "Condition.h"
#include "CndSpaceEnterArch.h"

namespace Missions
{
	struct CndSpaceEnter : Condition
	{
		CndSpaceEnterArchetypePtr archetype;

		CndSpaceEnter(const ConditionParent& parent, const CndSpaceEnterArchetypePtr conditionArchetype);
		~CndSpaceEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	extern std::unordered_set<CndSpaceEnter*> spaceEnterConditions;
}