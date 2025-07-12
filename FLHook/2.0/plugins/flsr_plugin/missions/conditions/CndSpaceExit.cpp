#include "CndSpaceExit.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndSpaceExit*> registeredConditions;

	CndSpaceExit::CndSpaceExit(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId) :
		Condition(parent),
		label(objNameOrLabel),
		systemId(systemId)
	{}

	CndSpaceExit::~CndSpaceExit()
	{
		Unregister();
	}

	void CndSpaceExit::Register()
	{
		registeredConditions.insert(this);
	}

	void CndSpaceExit::Unregister()
	{
		registeredConditions.erase(this);
	}

	bool CndSpaceExit::Matches(const uint clientId, const uint systemId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!systemId || systemId == systemId))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				return true;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId && (!systemId || systemId == systemId))
				{
					activator = object;
					return true;
				}
			}
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndSpaceExit
		{
			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
			{
				returncode = DEFAULT_RETURNCODE;

				if (!killedObject->is_player() || registeredConditions.empty())
					return;

				const std::unordered_set<Missions::CndSpaceExit*> currentConditions(registeredConditions);
				for (const auto& condition : currentConditions)
				{
					if (registeredConditions.contains(condition) && condition->Matches(killedObject->cobj->ownerPlayer, killedObject->cobj->system))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}