#include "CndBaseEnter.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndBaseEnter*> observedCndBaseEnter;

	CndBaseEnter::CndBaseEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint baseId) :
		Condition(parent),
		label(objNameOrLabel),
		baseId(baseId)
	{}

	CndBaseEnter::~CndBaseEnter()
	{
		Unregister();
	}

	void CndBaseEnter::Register()
	{
		observedCndBaseEnter.insert(this);
	}

	void CndBaseEnter::Unregister()
	{
		observedCndBaseEnter.erase(this);
	}

	bool CndBaseEnter::Matches(const uint clientId, const uint currentBaseId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!baseId || baseId == currentBaseId))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (!baseId || baseId == currentBaseId))
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
				returncode = DEFAULT_RETURNCODE;

				const std::unordered_set<Missions::CndBaseEnter*> currentConditions(observedCndBaseEnter);
				for (const auto& condition : currentConditions)
				{
					if (observedCndBaseEnter.contains(condition) && condition->Matches(clientId, baseId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}