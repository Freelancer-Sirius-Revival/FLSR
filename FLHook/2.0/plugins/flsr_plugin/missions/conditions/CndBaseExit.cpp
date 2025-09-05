#include "CndBaseExit.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndBaseExit*> observedCndBaseExit;

	CndBaseExit::CndBaseExit(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& baseIds) :
		Condition(parent),
		label(label),
		baseIds(baseIds)
	{}

	CndBaseExit::~CndBaseExit()
	{
		Unregister();
	}

	void CndBaseExit::Register()
	{
		observedCndBaseExit.insert(this);
	}

	void CndBaseExit::Unregister()
	{
		observedCndBaseExit.erase(this);
	}

	bool CndBaseExit::Matches(const uint clientId, const uint currentBaseId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (baseIds.empty() || baseIds.contains(currentBaseId)))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (baseIds.empty() || baseIds.contains(currentBaseId)))
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
		namespace CndBaseExit
		{
			void __stdcall BaseExit_AFTER(unsigned int baseId, unsigned int clientId)
			{
				returncode = DEFAULT_RETURNCODE;

				const std::unordered_set<Missions::CndBaseExit*> currentConditions(observedCndBaseExit);
				for (const auto& condition : currentConditions)
				{
					if (observedCndBaseExit.contains(condition) && condition->Matches(clientId, baseId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}