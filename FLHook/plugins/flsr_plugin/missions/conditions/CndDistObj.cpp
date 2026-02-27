#include "CndDistObj.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndDistObj*> observedCndDistObj;
	std::vector<CndDistObj*> orderedCndDistObj;

	CndDistObj::CndDistObj(const ConditionParent& parent,
								const uint objNameOrLabel,
								const DistanceCondition condition,
								const float distance,
								const uint otherObjNameOrLabel) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		condition(condition),
		distance(distance),
		otherObjNameOrLabel(otherObjNameOrLabel)
	{}

	CndDistObj::~CndDistObj()
	{
		Unregister();
	}

	void CndDistObj::Register()
	{
		if (observedCndDistObj.insert(this).second)
			orderedCndDistObj.push_back(this);
	}

	void CndDistObj::Unregister()
	{
		observedCndDistObj.erase(this);
		if (const auto it = std::find(orderedCndDistObj.begin(), orderedCndDistObj.end(), this); it != orderedCndDistObj.end())
			orderedCndDistObj.erase(it);
	}

	bool CndDistObj::IsDistanceMatching(const CObject* objA, uint objIdB) const
	{
		IObjRW* inspectB;
		StarSystem* starSystem;
		if (!GetShipInspect(objIdB, inspectB, starSystem))
			return false;
		if (objA->system != inspectB->cobj->system)
			return false;
		const bool inside = distance - HkDistance3D(objA->vPos, inspectB->cobj->vPos) > 0.0f;
		return (condition == CndDistObj::DistanceCondition::Inside && inside) || (condition == CndDistObj::DistanceCondition::Outside && !inside);
	}

	bool CndDistObj::IsDistanceMatchingToOthers(uint objId)
	{
		IObjRW* inspectA;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspectA, starSystem) || inspectA->cobj->objectClass != CObject::CSHIP_OBJECT)
			return false;

		// Try to find any solar in the entire game first.
		int reputationId;
		pub::SpaceObj::GetSolarRep(otherObjNameOrLabel, reputationId);
		// Solar IDs are the exact same as their reputation ID
		if (reputationId != 0 && otherObjNameOrLabel == reputationId)
			return IsDistanceMatching(inspectA->cobj, otherObjNameOrLabel);

		const auto& mission = missions.at(parent.missionId);
		if (const auto& objectByName = mission.objectIdsByName.find(otherObjNameOrLabel); objectByName != mission.objectIdsByName.end())
		{
			return IsDistanceMatching(inspectA->cobj, objectByName->second);
		}
		if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (labelObject.type == MissionObjectType::Client)
				{
					uint shipId;
					pub::Player::GetShip(labelObject.id, shipId);
					return shipId && IsDistanceMatching(inspectA->cobj, shipId);
				}
				else
				{
					return IsDistanceMatching(inspectA->cobj, labelObject.id);
				}
			}
		}
		return false;
	}

	bool CndDistObj::Matches(const MissionObject& object)
	{
		const auto& mission = missions.at(parent.missionId);
		if (object.type == MissionObjectType::Client && objNameOrLabel == Stranger && !mission.clientIds.contains(object.id))
		{
			uint shipId;
			pub::Player::GetShip(object.id, shipId);
			if (shipId && IsDistanceMatchingToOthers(shipId))
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
				if (objectByName->second == object.id && IsDistanceMatchingToOthers(object.id))
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
						if (shipId && IsDistanceMatchingToOthers(shipId))
						{
							activator = object;
							return true;
						}
						return false;
					}
					else
					{
						if (IsDistanceMatchingToOthers(object.id))
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
		namespace CndDistObj
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

				const auto currentConditions(orderedCndDistObj);
				for (const auto& condition : currentConditions)
				{
					if (!observedCndDistObj.contains(condition))
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