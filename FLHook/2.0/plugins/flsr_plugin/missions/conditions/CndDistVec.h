#pragma once
#include "Condition.h"
#include "CndDistVecArch.h"

namespace Missions
{
	struct DistVecMatchEntry
	{
		Vector position;
		uint objId;
		uint clientId;
	};

	struct CndDistVec : Condition
	{
		CndDistVecArchetypePtr archetype;

		CndDistVec(Trigger* parentTrigger, const CndDistVecArchetypePtr conditionArchetype);
		void Register();
		void Unregister();
		bool Matches(std::vector<DistVecMatchEntry>& entries);
	};

	extern std::unordered_set<CndDistVec*> distVecConditions;
}