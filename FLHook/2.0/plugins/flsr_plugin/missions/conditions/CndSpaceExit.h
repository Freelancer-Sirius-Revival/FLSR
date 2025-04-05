#pragma once
#include "Condition.h"
#include "CndSpaceExitArch.h"

namespace Missions
{
	struct CndSpaceExit : Condition
	{
		CndSpaceExitArchetypePtr archetype;

		CndSpaceExit(const ConditionParent& parent, const CndSpaceExitArchetypePtr conditionArchetype);
		~CndSpaceExit();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	extern std::unordered_set<CndSpaceExit*> spaceExitConditions;
}