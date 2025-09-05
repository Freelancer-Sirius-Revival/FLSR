#include "CndSystemSpaceExit.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndSystemSpaceExit*> observedCndSystemSpaceExit;

	CndSystemSpaceExit::CndSystemSpaceExit(const ConditionParent& parent, const uint label, const SystemExitCondition condition, const std::unordered_set<uint>& systemIds) :
		Condition(parent),
		label(label),
		condition(condition),
		systemIds(systemIds)
	{}

	CndSystemSpaceExit::~CndSystemSpaceExit()
	{
		Unregister();
	}

	void CndSystemSpaceExit::Register()
	{
		observedCndSystemSpaceExit.insert(this);
	}

	void CndSystemSpaceExit::Unregister()
	{
		observedCndSystemSpaceExit.erase(this);
	}

	bool CndSystemSpaceExit::Matches(const uint clientId, const SystemExitCondition reason)
	{
		if (condition != SystemExitCondition::Any && condition != reason)
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
		namespace CndSystemSpaceExit
		{
			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
			{
				returncode = DEFAULT_RETURNCODE;

				if (!killedObject->is_player())
					return;

				const std::unordered_set<Missions::CndSystemSpaceExit*> currentConditions(observedCndSystemSpaceExit);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemSpaceExit.contains(condition) && condition->Matches(killedObject->cobj->ownerPlayer, killed ? Missions::CndSystemSpaceExit::SystemExitCondition::Explode : Missions::CndSystemSpaceExit::SystemExitCondition::Vanish))
						condition->ExecuteTrigger();
				}
			}

			void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
			{
				returncode = DEFAULT_RETURNCODE;

				const std::unordered_set<Missions::CndSystemSpaceExit*> currentConditions(observedCndSystemSpaceExit);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemSpaceExit.contains(condition) && condition->Matches(clientId, Missions::CndSystemSpaceExit::SystemExitCondition::Dock))
						condition->ExecuteTrigger();
				}
			}

			void __stdcall SystemSwitchOutComplete(unsigned int objId, unsigned int clientId)
			{
				returncode = DEFAULT_RETURNCODE;

				const std::unordered_set<Missions::CndSystemSpaceExit*> currentConditions(observedCndSystemSpaceExit);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemSpaceExit.contains(condition) && condition->Matches(clientId, Missions::CndSystemSpaceExit::SystemExitCondition::Jump))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}