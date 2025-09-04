#include "CndJumpInComplete.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndJumpInComplete*> observedCndJumpInComplete;

	CndJumpInComplete::CndJumpInComplete(const ConditionParent& parent, const uint label, const std::unordered_set<uint> systemIds) :
		Condition(parent),
		label(label),
		systemIds(systemIds)
	{}

	CndJumpInComplete::~CndJumpInComplete()
	{
		Unregister();
	}

	void CndJumpInComplete::Register()
	{
		observedCndJumpInComplete.insert(this);
	}

	void CndJumpInComplete::Unregister()
	{
		observedCndJumpInComplete.erase(this);
	}

	bool CndJumpInComplete::Matches(const uint clientId, const uint currentSystemId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (systemIds.empty() || systemIds.contains(currentSystemId)))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (systemIds.empty() || systemIds.contains(currentSystemId)))
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
		namespace CndJumpInComplete
		{
			void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId)
			{
				returncode = DEFAULT_RETURNCODE;

				const uint clientId = HkGetClientIDByShip(shipId);
				if (!clientId)
					return;

				const std::unordered_set<Missions::CndJumpInComplete*> currentConditions(observedCndJumpInComplete);
				for (const auto& condition : currentConditions)
				{
					if (observedCndJumpInComplete.contains(condition) && condition->Matches(clientId, systemId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}