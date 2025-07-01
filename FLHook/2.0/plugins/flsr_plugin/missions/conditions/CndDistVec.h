#pragma once
#include "Condition.h"
#include "CndDistVecArch.h"

namespace Missions
{
	struct CndDistVec : Condition
	{
		CndDistVecArchetypePtr archetype;

		CndDistVec(const ConditionParent& parent, const CndDistVecArchetypePtr conditionArchetype);
		~CndDistVec();
		void Register();
		void Unregister();
		bool Matches();
	};

	void Cnd_DistVec_Elapse_Time_AFTER(const float seconds);
}