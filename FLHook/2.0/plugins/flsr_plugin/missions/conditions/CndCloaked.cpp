#include "CndCloaked.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndCloaked*> observedCndCloaked;
	std::vector<CndCloaked*> orderedCndCloaked;

	CndCloaked::CndCloaked(const ConditionParent& parent, const uint objNameOrLabel, const bool cloaked) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		cloaked(cloaked)
	{}

	CndCloaked::~CndCloaked()
	{
		Unregister();
	}

	void CndCloaked::Register()
	{
		if (observedCndCloaked.insert(this).second)
			orderedCndCloaked.push_back(this);
	}

	void CndCloaked::Unregister()
	{
		observedCndCloaked.erase(this);
		if (const auto it = std::find(orderedCndCloaked.begin(), orderedCndCloaked.end(), this); it != orderedCndCloaked.end())
			orderedCndCloaked.erase(it);
	}

	static bool IsCloaked(uint objId)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return false;
		return reinterpret_cast<CEqObj*>(inspect->cobj)->get_cloak_percentage() == 1.0f;
	}

	bool CndCloaked::Matches(const MissionObject& object)
	{
		const auto& mission = missions.at(parent.missionId);
		if (object.type == MissionObjectType::Client && objNameOrLabel == Stranger && !mission.clientIds.contains(object.id))
		{
			uint shipId;
			pub::Player::GetShip(object.id, shipId);
			if (shipId && IsCloaked(shipId) == cloaked)
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
				if (objectByName->second == object.id && IsCloaked(object.id) == cloaked)
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
						if (shipId && IsCloaked(shipId) == cloaked)
						{
							activator = object;
							return true;
						}
						return false;
					}
					else
					{
						if (IsCloaked(object.id) == cloaked)
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
		namespace CndCloaked
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

				const auto currentConditions(orderedCndCloaked);
				for (const auto& condition : currentConditions)
				{
					if (!observedCndCloaked.contains(condition))
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