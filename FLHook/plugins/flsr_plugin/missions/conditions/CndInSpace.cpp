#include "CndInSpace.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndInSpace*> observedCndInSpace;
	std::vector<CndInSpace*> orderedCndInSpace;

	CndInSpace::CndInSpace(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& systemIds) :
		Condition(parent),
		label(label),
		systemIds(systemIds)
	{}

	CndInSpace::~CndInSpace()
	{
		Unregister();
	}

	void CndInSpace::Register()
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
		if (observedCndInSpace.insert(this).second)
			orderedCndInSpace.push_back(this);
	}

	void CndInSpace::Unregister()
	{
		observedCndInSpace.erase(this);
		if (const auto it = std::find(orderedCndInSpace.begin(), orderedCndInSpace.end(), this); it != orderedCndInSpace.end())
			orderedCndInSpace.erase(it);
	}

	bool CndInSpace::Matches(const uint clientId)
	{
		uint currentSystemId;
		pub::Player::GetSystem(clientId, currentSystemId);
		uint currentBaseId;
		pub::Player::GetBase(clientId, currentBaseId);
		if (!currentSystemId || currentBaseId)
			return false;

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
		namespace CndInSpace
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
			{
				const std::unordered_set<Missions::CndInSpace*> currentConditions(observedCndInSpace);
				for (const auto& condition : currentConditions)
				{
					if (observedCndInSpace.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall SystemSwitchOutComplete_AFTER(unsigned int objId, unsigned int clientId)
			{
				const auto currentConditions(orderedCndInSpace);
				for (const auto& condition : currentConditions)
				{
					if (observedCndInSpace.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}