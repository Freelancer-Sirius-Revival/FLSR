#pragma once
#include "Condition.h"

namespace Missions
{
	class CndTimer : public Condition
	{
	private:
		const float lowerTimeInS;
		const float upperTimeInS;

		float targetTimeInS;
		float passedTimeInS;

	public:
		CndTimer(const ConditionParent& parent, const float lowerTimeInS, const float upperTimeInS);
		~CndTimer();
		void Register();
		void Unregister();
		bool Matches(const float elapsedTimeInS);
	};

	extern std::unordered_set<CndTimer*> timerConditions;
}