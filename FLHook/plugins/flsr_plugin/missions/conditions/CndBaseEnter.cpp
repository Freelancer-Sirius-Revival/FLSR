#include "CndBaseEnter.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndBaseEnter*> observedCndBaseEnter;
	std::vector<CndBaseEnter*> orderedCndBaseEnter;

	CndBaseEnter::CndBaseEnter(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& baseIds) :
		Condition(parent),
		label(label),
		baseIds(baseIds)
	{}

	CndBaseEnter::~CndBaseEnter()
	{
		Unregister();
	}

	void CndBaseEnter::Register()
	{
		if (observedCndBaseEnter.insert(this).second)
			orderedCndBaseEnter.push_back(this);
	}

	void CndBaseEnter::Unregister()
	{
		observedCndBaseEnter.erase(this);
		if (const auto it = std::find(orderedCndBaseEnter.begin(), orderedCndBaseEnter.end(), this); it != orderedCndBaseEnter.end())
			orderedCndBaseEnter.erase(it);
	}

	bool CndBaseEnter::Matches(const uint clientId, const uint currentBaseId)
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
		namespace CndBaseEnter
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
			{
				const auto currentConditions(orderedCndBaseEnter);
				for (const auto& condition : currentConditions)
				{
					if (observedCndBaseEnter.contains(condition) && condition->Matches(clientId, baseId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}