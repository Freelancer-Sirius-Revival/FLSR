#pragma once
#include "Condition.h"

namespace Missions
{
	class CndDistVec : public Condition
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
		const Vector position;
		const float distance;
		const uint systemId;

		bool IsDistanceMatching(uint objId);

	public:
		CndDistVec(const ConditionParent& parent,
					const uint objNameOrLabel,
					const DistanceCondition condition,
					const Vector& position,
					const float distance,
					const uint systemId);
		~CndDistVec();
		void Register();
		void Unregister();
		bool Matches(const MissionObject& object);
	};

	namespace Hooks
	{
		namespace CndDistVec
		{
			void __stdcall Elapse_Time_AFTER(float seconds);
		}
	}
}