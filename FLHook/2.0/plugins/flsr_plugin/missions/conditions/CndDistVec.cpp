#include "CndDistVec.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndDistVec*> distVecConditions;

	CndDistVec::CndDistVec(const ConditionParent& parent,
							const uint objNameOrLabel,
							const DistanceCondition condition,
							const Vector& position,
							const float distance,
							const uint systemId) :
							Condition(parent),
		objNameOrLabel(objNameOrLabel),
		condition(condition),
		position(position),
		distance(distance),
		systemId(systemId)
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

	static bool IsInside(const DistVecMatchEntry& entry, const CndDistVec& condition)
	{
		const bool inside = condition.distance - HkDistance3D(condition.position, entry.position) > 0.0f;
		return (condition.condition == CndDistVec::DistanceCondition::Inside && inside) || (condition.condition == CndDistVec::DistanceCondition::Outside && !inside);
	}

	bool CndDistVec::Matches(const std::unordered_map<uint, DistVecMatchEntry>& clientsByClientId, const std::unordered_map<uint, DistVecMatchEntry>& objectsByObjId)
	{
		const auto& mission = missions.at(parent.missionId);
		std::unordered_set<uint> validClientIds;
		std::unordered_set<uint> validObjIds;
		bool strangerRequested = objNameOrLabel == Stranger;
		if (strangerRequested)
		{
			validClientIds = mission.clientIds;
		}
		else if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
		{
			validObjIds.insert(objectByName->second);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
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
			if (entry.second.systemId == systemId && ((strangerRequested && !validClientIds.contains(entry.first)) || (!strangerRequested && validClientIds.contains(entry.first))) && IsInside(entry.second, *this))
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
				if (entry.second.systemId == systemId && validObjIds.contains(entry.first) && IsInside(entry.second, *this))
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