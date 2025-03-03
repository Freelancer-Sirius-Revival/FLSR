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

	static bool isInside(const DistVecMatchEntry& entry, const CndDistVecArchetypePtr& archetype)
	{
		if (entry.systemId != archetype->systemId)
			return false;
		const bool inside = archetype->distance - HkDistance3D(archetype->position, entry.position) > 0.0f;
		return (archetype->type == DistanceCondition::Inside && inside) || (archetype->type == DistanceCondition::Outside && !inside);
	}

	bool CndDistVec::Matches(const std::unordered_map<uint, DistVecMatchEntry>& clientsByClientId, const std::unordered_map<uint, DistVecMatchEntry>& objectsByObjId)
	{
		const std::wstring outputPretext = stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Cnd_DistVec " + std::to_wstring(archetype->objNameOrLabel);
		for (const uint clientId : trigger->mission->clientIds)
		{
			if (const auto& entry = clientsByClientId.find(clientId); entry != clientsByClientId.end() && isInside(entry->second, archetype))
			{
				activator.type = MissionObjectType::Client;
				activator.id = entry->first;
				ConPrint(outputPretext + L" client[" + std::to_wstring(activator.id) + L"]\n");
				return true;
			}
		}
		for (const uint objId : trigger->mission->objectIds)
		{
			if (const auto& entry = objectsByObjId.find(objId); entry != objectsByObjId.end() && isInside(entry->second, archetype))
			{
				activator.type = MissionObjectType::Object;
				activator.id = entry->first;
				ConPrint(outputPretext + L" client[" + std::to_wstring(activator.id) + L"]\n");
				return true;
			}
		}
		return false;
	}
}