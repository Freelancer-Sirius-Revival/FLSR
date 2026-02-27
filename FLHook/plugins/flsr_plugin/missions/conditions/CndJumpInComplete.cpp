#include "CndJumpInComplete.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndJumpInComplete*> observedCndJumpInComplete;
	std::vector<CndJumpInComplete*> orderedCndJumpInComplete;

	CndJumpInComplete::CndJumpInComplete(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& systemIds) :
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
		if (observedCndJumpInComplete.insert(this).second)
			orderedCndJumpInComplete.push_back(this);
	}

	void CndJumpInComplete::Unregister()
	{
		observedCndJumpInComplete.erase(this);
		if (const auto it = std::find(orderedCndJumpInComplete.begin(), orderedCndJumpInComplete.end(), this); it != orderedCndJumpInComplete.end())
			orderedCndJumpInComplete.erase(it);
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
				const uint clientId = HkGetClientIDByShip(shipId);
				if (!clientId)
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}

				const auto currentConditions(orderedCndJumpInComplete);
				for (const auto& condition : currentConditions)
				{
					if (observedCndJumpInComplete.contains(condition) && condition->Matches(clientId, systemId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}