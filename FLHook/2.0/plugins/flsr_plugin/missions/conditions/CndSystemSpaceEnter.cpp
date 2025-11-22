#include "CndSystemSpaceEnter.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndSystemSpaceEnter*> observedCndSystemSpaceEnter;
	std::vector<CndSystemSpaceEnter*> orderedCndSystemSpaceEnter;

	CndSystemSpaceEnter::CndSystemSpaceEnter(const ConditionParent& parent, const uint label, const SystemEnterCondition condition, const std::unordered_set<uint>& systemIds) :
		Condition(parent),
		label(label),
		condition(condition),
		systemIds(systemIds)
	{}

	CndSystemSpaceEnter::~CndSystemSpaceEnter()
	{
		Unregister();
	}

	void CndSystemSpaceEnter::Register()
	{
		if (observedCndSystemSpaceEnter.insert(this).second)
			orderedCndSystemSpaceEnter.push_back(this);
	}

	void CndSystemSpaceEnter::Unregister()
	{
		observedCndSystemSpaceEnter.erase(this);
		if (const auto it = std::find(orderedCndSystemSpaceEnter.begin(), orderedCndSystemSpaceEnter.end(), this); it != orderedCndSystemSpaceEnter.end())
			orderedCndSystemSpaceEnter.erase(it);
	}

	bool CndSystemSpaceEnter::Matches(const uint clientId, const SystemEnterCondition reason)
	{
		if (condition != SystemEnterCondition::Any && condition != reason)
			return false;

		uint currentSystemId;
		pub::Player::GetSystem(clientId, currentSystemId);

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
		namespace CndSystemSpaceEnter
		{
			std::unordered_set<uint> undockedClientIds;

			void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
			{
				undockedClientIds.insert(clientId);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall CharacterInfoReq(unsigned int clientId, bool p2)
			{
				undockedClientIds.erase(clientId);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2)
			{
				undockedClientIds.erase(clientId);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
			{
				const auto reason = undockedClientIds.contains(clientId) ? Missions::CndSystemSpaceEnter::SystemEnterCondition::Launch : Missions::CndSystemSpaceEnter::SystemEnterCondition::Spawn;
				const auto currentConditions(orderedCndSystemSpaceEnter);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemSpaceEnter.contains(condition) && condition->Matches(clientId, reason))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall SystemSwitchOutComplete_AFTER(unsigned int objId, unsigned int clientId)
			{
				const auto currentConditions(orderedCndSystemSpaceEnter);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemSpaceEnter.contains(condition) && condition->Matches(clientId, Missions::CndSystemSpaceEnter::SystemEnterCondition::Jump))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}