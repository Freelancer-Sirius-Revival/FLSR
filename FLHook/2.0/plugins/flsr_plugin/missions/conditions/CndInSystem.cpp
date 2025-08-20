#include "CndInSystem.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndInSystem*> observedCndInSystem;

	CndInSystem::CndInSystem(const ConditionParent& parent, const uint label, const uint systemId) :
		Condition(parent),
		label(label),
		systemId(systemId)
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
			if (Matches(playerData->iOnlineID, playerData->iSystemID))
			{
				ExecuteTrigger();
				return;
			}
		}
		observedCndInSystem.insert(this);
	}

	void CndInSystem::Unregister()
	{
		observedCndInSystem.erase(this);
	}

	bool CndInSystem::Matches(const uint clientId, const uint currentSystemId)
	{
		if (!currentSystemId)
			return false;

		uint shipId;
		pub::Player::GetShip(clientId, shipId);
		if (!shipId) // When in space, the player always must have a ship ID.
			return false;

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
		namespace CndInSystem
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
			{
				returncode = DEFAULT_RETURNCODE;

				if (observedCndInSystem.empty())
					return;
				uint systemId;
				pub::Player::GetSystem(clientId, systemId);

				const std::unordered_set<Missions::CndInSystem*> currentConditions(observedCndInSystem);
				for (const auto& condition : currentConditions)
				{
					if (observedCndInSystem.contains(condition) && condition->Matches(clientId, systemId))
						condition->ExecuteTrigger();
				}
			}

			void __stdcall JumpInComplete_AFTER(unsigned int systemId, unsigned int shipId)
			{
				returncode = DEFAULT_RETURNCODE;

				if (observedCndInSystem.empty())
					return;
				const uint clientId = HkGetClientIDByShip(shipId);
				if (!clientId)
					return;

				const std::unordered_set<Missions::CndInSystem*> currentConditions(observedCndInSystem);
				for (const auto& condition : currentConditions)
				{
					if (observedCndInSystem.contains(condition) && condition->Matches(clientId, systemId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}