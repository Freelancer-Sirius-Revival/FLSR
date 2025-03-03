#include "CndDistVec.h"

namespace Missions
{
	std::unordered_set<CndDistVec*> distVecConditions;

	CndDistVec::CndDistVec(Trigger* parentTrigger, const CndDistVecArchetypePtr conditionArchetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_DistVec),
		archetype(conditionArchetype)
	{}

	void CndDistVec::Register()
	{
		distVecConditions.insert(this);
	}

	void CndDistVec::Unregister()
	{
		distVecConditions.erase(this);
	}

	bool CndDistVec::Matches(std::vector<DistVecMatchEntry>& entries)
	{
		for (auto& entry : entries)
		{
			const bool inside = archetype->distance - HkDistance3D(archetype->position, entry.position) > 0.0f;
			if ((archetype->type == DistanceCondition::INSIDE && inside) || (archetype->type == DistanceCondition::OUTSIDE && !inside))
			{
				activator.clientId = entry.clientId;
				activator.objId = entry.objId;
				ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_DistVec " + std::to_wstring(archetype->objNameOrLabel) + L" client[" + std::to_wstring(activator.clientId) + L"] obj[" + std::to_wstring(activator.objId) + L"]\n");
				return true;
			}
		}
		return false;
	}
}