#include "CndInZone.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndInZone*> observedCndInZone;
	std::vector<CndInZone*> orderedCndInZone;

	CndInZone::CndInZone(const ConditionParent& parent, const uint objNameOrLabel, const std::unordered_set<uint>& zoneIds) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		zoneIds(zoneIds)
	{
		for (const auto& zoneId : zoneIds)
			systemIdByZoneIds.insert({ zoneId, pub::Zone::GetSystem(zoneId) });
	}

	CndInZone::~CndInZone()
	{
		Unregister();
	}

	void CndInZone::Register()
	{
		if (observedCndInZone.insert(this).second)
			orderedCndInZone.push_back(this);
	}

	void CndInZone::Unregister()
	{
		observedCndInZone.erase(this);
		if (const auto it = std::find(orderedCndInZone.begin(), orderedCndInZone.end(), this); it != orderedCndInZone.end())
			orderedCndInZone.erase(it);
	}

	bool CndInZone::IsInZone(uint objId)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem) || inspect->cobj->objectClass != CObject::CSHIP_OBJECT)
			return false;
		Vector physicalPosition;
		float physicalRadius;
		inspect->get_physical_radius(physicalRadius, physicalPosition);

		for (const auto& zoneId : zoneIds)
		{
			if (systemIdByZoneIds.at(zoneId) != inspect->cobj->system)
				continue;
			if (pub::Zone::InZone(zoneId, physicalPosition, physicalRadius))
				return true;
		}
		return false;
	}

	bool CndInZone::Matches(const MissionObject& object)
	{
		const auto& mission = missions.at(parent.missionId);
		if (object.type == MissionObjectType::Client && objNameOrLabel == Stranger && !mission.clientIds.contains(object.id))
		{
			uint shipId;
			pub::Player::GetShip(object.id, shipId);
			if (shipId && IsInZone(shipId))
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
				if (objectByName->second == object.id && IsInZone(object.id))
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
						if (shipId && IsInZone(shipId))
						{
							activator = object;
							return true;
						}
						return false;
					}
					else
					{
						if (IsInZone(object.id))
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
		namespace CndInZone
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

				const auto currentConditions(orderedCndInZone);
				for (const auto& condition : currentConditions)
				{
					if (!observedCndInZone.contains(condition))
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