#include "CndSpaceEnter.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndSpaceEnter*> observedCndSpaceEnter;

	CndSpaceEnter::CndSpaceEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId) :
		Condition(parent),
		label(objNameOrLabel),
		systemId(systemId)
	{}

	CndSpaceEnter::~CndSpaceEnter()
	{
		Unregister();
	}

	void CndSpaceEnter::Register()
	{
		observedCndSpaceEnter.insert(this);
	}

	void CndSpaceEnter::Unregister()
	{
		observedCndSpaceEnter.erase(this);
	}

	bool CndSpaceEnter::Matches(const uint clientId, const uint currentSystemId)
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
		namespace CndSpaceEnter
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
			{
				returncode = DEFAULT_RETURNCODE;

				if (observedCndSpaceEnter.empty())
					return;
				uint systemId;
				pub::Player::GetSystem(clientId, systemId);

				const std::unordered_set<Missions::CndSpaceEnter*> currentConditions(observedCndSpaceEnter);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSpaceEnter.contains(condition) && condition->Matches(clientId, systemId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}