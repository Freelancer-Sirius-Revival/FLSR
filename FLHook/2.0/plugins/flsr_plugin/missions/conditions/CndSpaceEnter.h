#pragma once
#include "Condition.h"
#include "CndSpaceEnterArch.h"

namespace Missions
{
	struct CndSpaceEnter : Condition
	{
		CndSpaceEnterArchetypePtr archetype;

		CndSpaceEnter(Trigger* parentTrigger, const CndSpaceEnterArchetypePtr conditionArchetype);
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	extern std::unordered_set<CndSpaceEnter*> spaceEnterConditions;
}