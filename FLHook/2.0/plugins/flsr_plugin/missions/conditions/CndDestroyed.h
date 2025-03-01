#pragma once
#include <FLHook.h>
#include "Condition.h"
#include "CndDestroyedArch.h"

namespace Missions
{
	struct CndDestroyed : Condition
	{
		const std::string objNameOrLabel;
		int count;
		const int targetCount;
		const DestroyedCondition condition;
		const std::string killerNameOrLabel;

		CndDestroyed(Trigger* parentTrigger, const CndDestroyedArchetype* archetype);
		void Register();
		void Unregister();
		bool Matches(const IObjRW* killedObject, const bool killed, const uint killerId);
	};

	extern std::unordered_set<CndDestroyed*> destroyedConditions;
}