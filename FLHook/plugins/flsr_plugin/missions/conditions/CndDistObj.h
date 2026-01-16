#pragma once
#include "Condition.h"

namespace Missions
{
	class CndDistObj : public Condition
	{
	public:
		enum class DistanceCondition
		{
			Inside,
			Outside
		};

	private:
		const uint objNameOrLabel;
		const DistanceCondition condition;
		const float distance;
		const uint otherObjNameOrLabel;

		bool IsDistanceMatching(const CObject* objA, uint objIdB) const;
		bool IsDistanceMatchingToOthers(uint objId);

	public:
		CndDistObj(const ConditionParent& parent,
			const uint objNameOrLabel,
			const DistanceCondition condition,
			const float distance,
			const uint otherObjNameOrLabel);
		~CndDistObj();
		void Register();
		void Unregister();
		bool Matches(const MissionObject& object);
	};

	namespace Hooks
	{
		namespace CndDistObj
		{
			void __stdcall Elapse_Time_AFTER(float seconds);
		}
	}
}