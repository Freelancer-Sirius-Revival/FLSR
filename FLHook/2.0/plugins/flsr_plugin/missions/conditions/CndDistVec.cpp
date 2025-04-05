#include "CndDistVec.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndDistVec*> distVecConditions;

	CndDistVec::CndDistVec(const ConditionParent& parent, const CndDistVecArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_DistVec),
		archetype(conditionArchetype)
	{}

	CndDistVec::~CndDistVec()
	{
		Unregister();
	}

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
		auto& mission = missions.at(parent.missionId);
		std::unordered_set<uint> validClientIds;
		std::unordered_set<uint> validObjIds;
		bool strangerRequested = archetype->objNameOrLabel == Stranger;
		if (strangerRequested)
		{
			validClientIds = mission.clientIds;
		}
		else if (const auto& objectByName = mission.objectIdsByName.find(archetype->objNameOrLabel); objectByName != mission.objectIdsByName.end())
		{
			validObjIds.insert(objectByName->second);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
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
					return true;
				}
			}
		}
		return false;
	}
}