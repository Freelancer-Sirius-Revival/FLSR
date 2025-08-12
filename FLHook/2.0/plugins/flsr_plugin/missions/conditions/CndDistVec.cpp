#include "CndDistVec.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndDistVec*> observedCndDistVec;

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
		observedCndDistVec.insert(this);
	}

	void CndDistVec::Unregister()
	{
		observedCndDistVec.erase(this);
	}
	
	bool CndDistVec::IsDistanceMatching(uint objId)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem) || !(inspect->cobj->objectClass & CObject::CSHIP_OBJECT))
			return false;
		if (inspect->cobj->system != systemId)
			return false;
		const bool inside = distance - HkDistance3D(position, inspect->cobj->vPos) > 0.0f;
		return (condition == CndDistVec::DistanceCondition::Inside && inside) || (condition == CndDistVec::DistanceCondition::Outside && !inside);
	}

	bool CndDistVec::Matches(const MissionObject& object)
	{
		const auto& mission = missions.at(parent.missionId);
		if (object.type == MissionObjectType::Client && objNameOrLabel == Stranger && !mission.clientIds.contains(object.id))
		{
			uint shipId;
			pub::Player::GetShip(object.id, shipId);
			if (shipId && IsDistanceMatching(shipId))
			{
				activator = object;
				return true;
			}
			return false;
		}

		if ((object.type == MissionObjectType::Client && !mission.clientIds.contains(object.id)) ||
			(object.type == MissionObjectType::Object && !mission.objectIds.contains(object.id)))
		{
			return false;
		}

		if (object.type == MissionObjectType::Object)
		{
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				if (objectByName->second == object.id && IsDistanceMatching(object.id))
				{
					activator = object;
					return true;
				}
				return false;
			}
		}
		if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (object == labelObject)
				{
					if (object.type == MissionObjectType::Client)
					{
						uint shipId;
						pub::Player::GetShip(object.id, shipId);
						if (shipId && IsDistanceMatching(shipId))
						{
							activator = object;
							return true;
						}
						return false;
					}
					else
					{
						if (IsDistanceMatching(object.id))
						{
							activator = object;
							return true;
						}
						return false;
					}
				}
			}
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndDistVec
		{
			float elapsedTimeInSec = 0.0f;
			void __stdcall Elapse_Time_AFTER(float seconds)
			{
				returncode = DEFAULT_RETURNCODE;

				elapsedTimeInSec += seconds;
				if (elapsedTimeInSec < 0.02f)
					return;
				elapsedTimeInSec = 0.0f;

				const std::unordered_set<Missions::CndDistVec*> currentConditions(observedCndDistVec);
				for (const auto& condition : currentConditions)
				{
					if (!observedCndDistVec.contains(condition))
						continue;

					bool matchFound = false;
					struct PlayerData* playerData = 0;
					while (playerData = Players.traverse_active(playerData))
					{
						if (condition->Matches(MissionObject(MissionObjectType::Client, playerData->iOnlineID)))
						{
							condition->ExecuteTrigger();
							matchFound = true;
							break;
						}
					}
					if (!matchFound)
					{
						for (const uint objId : missions.at(condition->parent.missionId).objectIds)
						{
							if (condition->Matches(MissionObject(MissionObjectType::Object, objId)))
							{
								condition->ExecuteTrigger();
								break;
							}
						}
					}
				}
			}
		}
	}
}