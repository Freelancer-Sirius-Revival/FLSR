#include "CndInSystem.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndInSystem*> observedCndInSystem;
	std::vector<CndInSystem*> orderedCndInSystem;

	CndInSystem::CndInSystem(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& systemIds) :
		Condition(parent),
		label(label),
		systemIds(systemIds)
	{}

	CndInSystem::~CndInSystem()
	{
		Unregister();
	}

	void CndInSystem::Register()
	{
		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			if (Matches(playerData->iOnlineID))
			{
				ExecuteTrigger();
				return;
			}
		}
		if (observedCndInSystem.insert(this).second)
			orderedCndInSystem.push_back(this);
	}

	void CndInSystem::Unregister()
	{
		observedCndInSystem.erase(this);
		if (const auto it = std::find(orderedCndInSystem.begin(), orderedCndInSystem.end(), this); it != orderedCndInSystem.end())
			orderedCndInSystem.erase(it);
	}

	bool CndInSystem::Matches(const uint clientId)
	{
		uint currentSystemId;
		pub::Player::GetSystem(clientId, currentSystemId);

		if (!currentSystemId)
			return false;

		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && systemIds.contains(currentSystemId))
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
				if (object.type == MissionObjectType::Client && object.id == clientId && systemIds.contains(currentSystemId))
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
		namespace CndInSystem
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
			{
				const std::unordered_set<Missions::CndInSystem*> currentConditions(observedCndInSystem);
				for (const auto& condition : currentConditions)
				{
					if (observedCndInSystem.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
			{
				const std::unordered_set<Missions::CndInSystem*> currentConditions(observedCndInSystem);
				for (const auto& condition : currentConditions)
				{
					if (observedCndInSystem.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall SystemSwitchOutComplete_AFTER(unsigned int objId, unsigned int clientId)
			{
				const auto currentConditions(orderedCndInSystem);
				for (const auto& condition : currentConditions)
				{
					if (observedCndInSystem.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}