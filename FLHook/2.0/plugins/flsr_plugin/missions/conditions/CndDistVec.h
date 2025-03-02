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
		DistanceCondition type;
		std::string objNameOrLabel;
		Vector position;
		float distance;
		unsigned int systemId;

		CndDistVec(Trigger* parentTrigger, const CndDistVecArchetype* archetype);
		void Register();
		void Unregister();
		bool Matches(std::vector<DistVecMatchEntry>& entries);
	};

	extern std::unordered_set<CndDistVec*> distVecConditions;
}