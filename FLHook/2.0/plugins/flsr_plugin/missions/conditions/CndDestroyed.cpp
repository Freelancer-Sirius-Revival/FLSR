#include "CndDestroyed.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndDestroyed*> observedCndDestroyed;
	std::vector<CndDestroyed*> orderedCndDestroyed;

	CndDestroyed::CndDestroyed(const ConditionParent& parent,
								const uint objNameOrLabel,
								const DestroyCondition condition,
								const uint killerNameOrLabel,
								const int targetCount,
								const bool destroyedIsActivator) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		condition(condition),
		killerNameOrLabel(killerNameOrLabel),
		targetCount(targetCount),
		currentCount(0),
		destroyedIsActivator(destroyedIsActivator)
	{}

	CndDestroyed::~CndDestroyed()
	{
		Unregister();
	}

	void CndDestroyed::Register()
	{
		currentCount = 0;
		if (observedCndDestroyed.insert(this).second)
			orderedCndDestroyed.push_back(this);
	}

	void CndDestroyed::Unregister()
	{
		observedCndDestroyed.erase(this);
		if (const auto it = std::find(orderedCndDestroyed.begin(), orderedCndDestroyed.end(), this); it != orderedCndDestroyed.end())
			orderedCndDestroyed.erase(it);
	}

	bool CndDestroyed::Matches(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		const auto& mission = missions.at(parent.missionId);
		// If count -1 and the awaited objects do not exist, see this condition as fulfilled. "Stranger" also is fulfilled then because otherwise all players must leave server for fulfill.
		if (targetCount < 0 && (objNameOrLabel == Stranger || (!mission.objectIdsByName.contains(objNameOrLabel) && !mission.objectsByLabel.contains(objNameOrLabel))))
			return true;
		
		// Check destruction conditions.
		if ((condition == DestroyCondition::Vanish && killed) || (condition == DestroyCondition::Explode && !killed))
			return false;

		// Check if killed object is part of the mission.
		if (killedObject->is_player())
		{
			const bool containsPlayer = mission.clientIds.contains(killedObject->cobj->ownerPlayer);
			if ((objNameOrLabel == Stranger && containsPlayer) || !containsPlayer)
				return false;
		}
		else
		{
			if (!mission.objectIds.contains(killedObject->cobj->id))
				return false;
		}

		// Make sure the expected killer did the kill.
		if (killerNameOrLabel)
		{
			bool killerFound = false;
			if (const auto& objectByName = mission.objectIdsByName.find(killerNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				killerFound = objectByName->second == killerId;
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(killerNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.id == killerId)
					{
						killerFound = true;
						break;
					}
				}
			}
			if (!killerFound)
				return false;
		}

		byte foundObjectType = 0;
		if (killedObject->is_player() && objNameOrLabel == Stranger && !mission.clientIds.contains(killedObject->cobj->ownerPlayer))
		{
			foundObjectType = 1;
		}
		// Clients can only be addressed via Label.
		else if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
		{
			if (objectByName->second == killedObject->cobj->id)
			{
				foundObjectType = 2;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if ((object.type == MissionObjectType::Client && object.id == killedObject->cobj->ownerPlayer) || (object.type == MissionObjectType::Object && object.id == killedObject->cobj->id))
				{
					foundObjectType = 3;
					break;
				}
			}
		}

		if (foundObjectType == 0)
			return false;

		bool foundAll = false;
		if (targetCount < 0)
			foundAll = foundObjectType <= 2 || (foundObjectType == 3 && mission.objectsByLabel.at(objNameOrLabel).size() <= 1);
		else
			foundAll = ++currentCount >= targetCount;

		if (foundAll)
		{
			if (destroyedIsActivator)
			{
				activator = killedObject->is_player()
								? MissionObject(MissionObjectType::Client, killedObject->cobj->ownerPlayer)
								: MissionObject(MissionObjectType::Object, killedObject->get_id());
			}
			else
			{
				const uint clientId = HkGetClientIDByShip(killerId);
				activator.type = clientId ? MissionObjectType::Client : MissionObjectType::Object;
				activator.id = clientId ? clientId : killerId;
			}
			return true;
		}

		return false;
	}

	namespace Hooks
	{
		namespace CndDestroyed
		{
			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
			{
				const auto currentConditions(orderedCndDestroyed);
				for (const auto& condition : currentConditions)
				{
					if (observedCndDestroyed.contains(condition) && condition->Matches(killedObject, killed, killerId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}