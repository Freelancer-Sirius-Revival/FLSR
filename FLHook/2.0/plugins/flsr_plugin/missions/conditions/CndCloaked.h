#pragma once
#include "Condition.h"
#include "CndCloakedArch.h"

namespace Missions
{
	struct CndCloaked : Condition
	{
		CndCloakedArchetypePtr archetype;

		CndCloaked(const ConditionParent& parent, const CndCloakedArchetypePtr conditionArchetype);
		~CndCloaked();
		void Register();
		void Unregister();
		bool Matches(const MissionObject object);
	};

	extern std::unordered_set<CndCloaked*> cloakedConditions;
}