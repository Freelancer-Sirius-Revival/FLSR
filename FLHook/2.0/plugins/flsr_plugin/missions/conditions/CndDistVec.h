#pragma once
#include "Condition.h"
#include "CndDistVecArch.h"

namespace Missions
{
	struct DistVecMatchEntry
	{
		uint systemId;
		Vector position;
	};

	struct CndDistVec : Condition
	{
		CndDistVecArchetypePtr archetype;

		CndDistVec(Trigger* parentTrigger, const CndDistVecArchetypePtr conditionArchetype);
		void Register();
		void Unregister();
		bool Matches(const std::unordered_map<uint, DistVecMatchEntry>& clientsByClientId, const std::unordered_map<uint, DistVecMatchEntry>& objectsByObjId);
	};

	extern std::unordered_set<CndDistVec*> distVecConditions;
}