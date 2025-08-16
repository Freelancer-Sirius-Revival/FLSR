#include "CndSystemEnter.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndSystemEnter*> observedCndSystemEnter;

	CndSystemEnter::CndSystemEnter(const ConditionParent& parent, const uint label, const uint systemId) :
		Condition(parent),
		label(label),
		systemId(systemId)
	{}

	CndSystemEnter::~CndSystemEnter()
	{
		Unregister();
	}

	void CndSystemEnter::Register()
	{
		observedCndSystemEnter.insert(this);
	}

	void CndSystemEnter::Unregister()
	{
		observedCndSystemEnter.erase(this);
	}

	bool CndSystemEnter::Matches(const uint clientId, const uint currentSystemId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!systemId || systemId == currentSystemId))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && (!systemId || systemId == currentSystemId))
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
		namespace CndSystemEnter
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
			{
				returncode = DEFAULT_RETURNCODE;

				if (observedCndSystemEnter.empty())
					return;
				uint systemId;
				pub::Player::GetSystem(clientId, systemId);

				const std::unordered_set<Missions::CndSystemEnter*> currentConditions(observedCndSystemEnter);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemEnter.contains(condition) && condition->Matches(clientId, systemId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}