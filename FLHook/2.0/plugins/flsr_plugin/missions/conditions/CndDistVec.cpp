#include "CndDistVec.h"
#include "../Mission.h"
#include "../../Plugin.h"

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

	static bool IsDistanceMatching(const CSimple* cobj, const CndDistVecArchetypePtr& archetype)
	{
		const bool inside = archetype->distance - HkDistance3D(archetype->position, cobj->vPos) > 0.0f;
		return (archetype->type == DistanceCondition::Inside && inside) || (archetype->type == DistanceCondition::Outside && !inside);
	}

	bool CndDistVec::Matches()
	{
		const auto& mission = missions.at(parent.missionId);
		if (archetype->objNameOrLabel == Stranger)
		{
			PlayerData* playerData = 0;
			while (playerData = Players.traverse_active(playerData))
			{
				if (!mission.clientIds.contains(playerData->iOnlineID) && playerData->iShipID && playerData->iSystemID == archetype->systemId)
				{
					IObjRW* inspect;
					StarSystem* starSystem;
					if (GetShipInspect(playerData->iShipID, inspect, starSystem) && IsDistanceMatching(inspect->cobj, archetype))
					{
						activator = MissionObject(MissionObjectType::Client, playerData->iOnlineID);
						return true;
					}
				}
			}
		}
		else if (const auto& objectId = mission.objectIdsByName.find(archetype->objNameOrLabel); objectId != mission.objectIdsByName.end())
		{
			uint objId = objectId->second;
			IObjRW* inspect;
			StarSystem* starSystem;
			if (GetShipInspect(objId, inspect, starSystem) && !(inspect->cobj->objectClass & CObject::CSOLAR_OBJECT) && IsDistanceMatching(inspect->cobj, archetype))
			{
				activator = MissionObject(MissionObjectType::Object, objId);
				return true;
			}
		}
		else
		{
			const auto& objectsByLabelEntry = mission.objectsByLabel.find(archetype->objNameOrLabel);
			if (objectsByLabelEntry == mission.objectsByLabel.end())
				return false;

			for (const auto& objectEntry : objectsByLabelEntry->second)
			{
				uint objId;
				IObjRW* inspect;
				StarSystem* starSystem;
				if (objectEntry.type == MissionObjectType::Client)
				{
					pub::Player::GetShip(objectEntry.id, objId);
					if (objId && GetShipInspect(objId, inspect, starSystem) && IsDistanceMatching(inspect->cobj, archetype))
					{
						activator = objectEntry;
						return true;
					}
				}
				else
				{
					objId = objectEntry.id;
					if (GetShipInspect(objId, inspect, starSystem) && !(inspect->cobj->objectClass & CObject::CSOLAR_OBJECT) && IsDistanceMatching(inspect->cobj, archetype))
					{
						activator = objectEntry;
						return true;
					}
				}
			}
		}
		return false;
	}
	
	float elapsedTimeSinceLastUpdate = 0.0f;
	void Cnd_DistVec_Elapse_Time_AFTER(const float seconds)
	{
		elapsedTimeSinceLastUpdate += seconds;
		if (elapsedTimeSinceLastUpdate < 0.02f)
			return;
		elapsedTimeSinceLastUpdate = 0.0f;

		// Trigger execution can delete conditions on the fly. Make a copy with checks to avoid any issues.
		const std::unordered_set<CndDistVec*> distVecConditionsCopy(distVecConditions);
		for (const auto& condition : distVecConditionsCopy)
		{
			if (distVecConditions.contains(condition) && condition->Matches())
				condition->ExecuteTrigger();
		}
	}
}