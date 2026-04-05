#pragma once
#include "Condition.h"

namespace Missions
{
	class CndTimer : public Condition
	{
	private:
		const uint activatorLabel;
		const float lowerTimeInS;
		const float upperTimeInS;

		float targetTimeInS;
		float passedTimeInS;

	public:
		CndTimer(const ConditionParent& parent, const float lowerTimeInS, const float upperTimeInS, const uint activatorLabel = 0);
		~CndTimer();
		ConditionPtr Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const;
		void Register();
		void Unregister();
		bool Matches(const float elapsedTimeInS);
	};

	namespace Hooks
	{
		namespace CndTimer
		{
			void __stdcall Elapse_Time_AFTER(float seconds);
		}
	}
}