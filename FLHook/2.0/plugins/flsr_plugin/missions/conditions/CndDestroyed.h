#pragma once
#include <FLHook.h>
#include "Condition.h"

namespace Missions
{
	class CndDestroyed : public Condition
	{
	public:
		enum class DestroyCondition
		{
			ALL,
			SILENT,
			EXPLODE
		};

	private:
		const uint objNameOrLabel;
		const DestroyCondition condition;
		const uint killerNameOrLabel;
		const int targetCount;

		int currentCount;

	public:
		CndDestroyed(const ConditionParent& parent,
						const uint objNameOrLabel,
						const DestroyCondition condition,
						const uint killerNameOrLabel,
						const int targetCount);
		~CndDestroyed();
		void Register();
		void Unregister();
		bool Matches(const IObjRW* killedObject, const bool killed, const uint killerId);
	};

	extern std::unordered_set<CndDestroyed*> destroyedConditions;
}