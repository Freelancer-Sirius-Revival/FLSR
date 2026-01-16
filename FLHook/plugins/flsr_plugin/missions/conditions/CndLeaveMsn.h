#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndLeaveMsn : public Condition
	{
	private:
		const uint label;

	public:
		CndLeaveMsn(const ConditionParent& parent, const uint label);
		~CndLeaveMsn();
		void Register();
		void Unregister();
		bool Matches(const uint clientId);
	};

	namespace Hooks
	{
		namespace CndLeaveMsn
		{
			void EvaluateLeaveMission(const uint missionId, const uint clientId);
		}
	}
}