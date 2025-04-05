#pragma once
#include <FLHook.h>
#include "Condition.h"
#include "CndDestroyedArch.h"

namespace Missions
{
	struct CndDestroyed : Condition
	{
		const CndDestroyedArchetypePtr archetype;
		int count;

		CndDestroyed(const ConditionParent& parent, const CndDestroyedArchetypePtr conditionArchetype);
		~CndDestroyed();
		void Register();
		void Unregister();
		bool Matches(const IObjRW* killedObject, const bool killed, const uint killerId);
	};

	extern std::unordered_set<CndDestroyed*> destroyedConditions;
}