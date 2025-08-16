#pragma once
#include "Condition.h"

namespace Missions
{
	class CndDestroyed : public Condition
	{
	public:
		enum class DestroyCondition
		{
			Any,
			Vanish,
			Explode
		};

	private:
		const uint objNameOrLabel;
		const DestroyCondition condition;
		const uint killerNameOrLabel;
		const int targetCount;
		const bool destroyedIsActivator;

		int currentCount;

	public:
		CndDestroyed(const ConditionParent& parent,
						const uint objNameOrLabel,
						const DestroyCondition condition,
						const uint killerNameOrLabel,
						const int targetCount,
						const bool destroyedIsActivator);
		~CndDestroyed();
		void Register();
		void Unregister();
		bool Matches(const IObjRW* killedObject, const bool killed, const uint killerId);
	};

	namespace Hooks
	{
		namespace CndDestroyed
		{
			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
		}
	}
}