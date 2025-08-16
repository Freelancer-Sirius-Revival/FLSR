#include "CndSystemExit.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndSystemExit*> observedCndSystemExit;

	CndSystemExit::CndSystemExit(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId) :
		Condition(parent),
		label(objNameOrLabel),
		systemId(systemId)
	{}

	CndSystemExit::~CndSystemExit()
	{
		Unregister();
	}

	void CndSystemExit::Register()
	{
		observedCndSystemExit.insert(this);
	}

	void CndSystemExit::Unregister()
	{
		observedCndSystemExit.erase(this);
	}

	bool CndSystemExit::Matches(const uint clientId, const uint systemId)
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
		namespace CndSystemExit
		{
			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
			{
				returncode = DEFAULT_RETURNCODE;

				if (!killedObject->is_player() || observedCndSystemExit.empty())
					return;

				const std::unordered_set<Missions::CndSystemExit*> currentConditions(observedCndSystemExit);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemExit.contains(condition) && condition->Matches(killedObject->cobj->ownerPlayer, killedObject->cobj->system))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}