#include "CndDistVec.h"

namespace Missions
{
	std::unordered_set<CndDistVec*> distVecConditions;

	CndDistVec::CndDistVec(Trigger* parentTrigger, const CndDistVecArchetype* archetype) :
		Condition(parentTrigger, TriggerCondition::Cnd_DistVec),
		type(archetype->type),
		objNameOrLabel(archetype->objNameOrLabel),
		position(archetype->position),
		distance(archetype->distance),
		systemId(archetype->systemId)
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
			const bool inside = distance - HkDistance3D(position, entry.position) > 0.0f;
			if ((type == DistanceCondition::INSIDE && inside) || (type == DistanceCondition::OUTSIDE && !inside))
			{
				activator.clientId = entry.clientId;
				activator.objId = entry.objId;
				ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Cnd_DistVec " + stows(objNameOrLabel) + L" client[" + std::to_wstring(activator.clientId) + L"] obj[" + std::to_wstring(activator.objId) + L"]\n");
				return true;
			}
		}
		return false;
	}
}