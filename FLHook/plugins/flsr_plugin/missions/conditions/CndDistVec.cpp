#include "CndDistVec.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndDistVec*> observedCndDistVec;
	std::vector<CndDistVec*> orderedCndDistVec;

	CndDistVec::CndDistVec(const ConditionParent& parent,
							const uint objNameOrLabel,
							const DistanceCondition condition,
							const Vector& position,
							const float distance,
							const uint systemId,
							const std::string hardpoint) :
							Condition(parent),
		objNameOrLabel(objNameOrLabel),
		condition(condition),
		position(position),
		distance(distance),
		systemId(systemId),
		hardpoint(hardpoint)
	{}

	CndDistVec::~CndDistVec()
	{
		Unregister();
	}

	void CndDistVec::Register()
	{
		if (observedCndDistVec.insert(this).second)
			orderedCndDistVec.push_back(this);
	}

	void CndDistVec::Unregister()
	{
		observedCndDistVec.erase(this);
		if (const auto it = std::find(orderedCndDistVec.begin(), orderedCndDistVec.end(), this); it != orderedCndDistVec.end())
			orderedCndDistVec.erase(it);
	}
	
	bool CndDistVec::IsDistanceMatching(uint objId) const
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem) || inspect->cobj->objectClass != CObject::CSHIP_OBJECT)
			return false;
		if (inspect->cobj->system != systemId)
			return false;
		Vector objPosition;
		Matrix orientation;
		if (hardpoint.empty() || inspect->get_hardpoint(hardpoint.c_str(), &objPosition, &orientation) != 0)
			objPosition = inspect->cobj->vPos;
		const bool inside = distance - HkDistance3D(position, objPosition) > 0.0f;
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
				elapsedTimeInSec += seconds;
				if (elapsedTimeInSec < 0.02f)
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}
				elapsedTimeInSec = 0.0f;

				const auto currentConditions(orderedCndDistVec);
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
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}