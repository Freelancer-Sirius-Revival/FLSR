#include "CndDistVec.h"

namespace Missions
{
	std::unordered_set<CndDistVec*> distVecConditions;

	CndDistVec::CndDistVec(const ConditionParent& parent, const CndDistVecArchetypePtr conditionArchetype) :
		Condition(parent, TriggerCondition::Cnd_DistVec),
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
		const bool inside = archetype->distance - HkDistance3D(archetype->position, entry.position) > 0.0f;
		return (archetype->type == DistanceCondition::Inside && inside) || (archetype->type == DistanceCondition::Outside && !inside);
	}

	bool CndDistVec::Matches(const std::unordered_map<uint, DistVecMatchEntry>& clientsByClientId, const std::unordered_map<uint, DistVecMatchEntry>& objectsByObjId)
	{
		const std::wstring outputPretext = stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Cnd_DistVec " + std::to_wstring(archetype->objNameOrLabel);

		std::unordered_set<uint> validClientIds;
		std::unordered_set<uint> validObjIds;
		bool strangerRequested = archetype->objNameOrLabel == Stranger;
		if (strangerRequested)
		{
			validClientIds = missions[parent.missionId].clientIds;
		}
		else if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->objNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
		{
			validObjIds.insert(objectByName->second);
		}
		else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					validClientIds.insert(object.id);
				else
					validObjIds.insert(object.id);
			}
		}
		
		for (const auto& entry : clientsByClientId)
		{
			if (entry.second.systemId == archetype->systemId && ((strangerRequested && !validClientIds.contains(entry.first)) || (!strangerRequested && validClientIds.contains(entry.first))) && isInside(entry.second, archetype))
			{
				activator.type = MissionObjectType::Client;
				activator.id = entry.first;
				ConPrint(outputPretext + L" client[" + std::to_wstring(activator.id) + L"]\n");
				return true;
			}
		}
		if (!strangerRequested)
		{
			for (const auto& entry : objectsByObjId)
			{
				if (entry.second.systemId == archetype->systemId && validObjIds.contains(entry.first) && isInside(entry.second, archetype))
				{
					activator.type = MissionObjectType::Object;
					activator.id = entry.first;
					ConPrint(outputPretext + L" obj[" + std::to_wstring(activator.id) + L"]\n");
					return true;
				}
			}
		}
		return false;
	}
}