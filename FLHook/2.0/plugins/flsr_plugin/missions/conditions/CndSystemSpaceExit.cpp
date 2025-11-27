#include "CndSystemSpaceExit.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndSystemSpaceExit*> observedCndSystemSpaceExit;
	std::vector<CndSystemSpaceExit*> orderedCndSystemSpaceExit;

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
		if (observedCndSystemSpaceExit.insert(this).second)
			orderedCndSystemSpaceExit.push_back(this);
	}

	void CndSystemSpaceExit::Unregister()
	{
		observedCndSystemSpaceExit.erase(this);
		if (const auto it = std::find(orderedCndSystemSpaceExit.begin(), orderedCndSystemSpaceExit.end(), this); it != orderedCndSystemSpaceExit.end())
			orderedCndSystemSpaceExit.erase(it);
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
			void static LoopThroughConditions(const uint clientId, const Missions::CndSystemSpaceExit::SystemExitCondition reason)
			{
				const auto currentConditions(orderedCndSystemSpaceExit);
				for (const auto& condition : currentConditions)
				{
					if (observedCndSystemSpaceExit.contains(condition) && condition->Matches(clientId, reason))
						condition->ExecuteTrigger();
				}
			}

			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
			{
				if (!killedObject->is_player())
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}

				LoopThroughConditions(killedObject->cobj->ownerPlayer, killed ? Missions::CndSystemSpaceExit::SystemExitCondition::Explode : Missions::CndSystemSpaceExit::SystemExitCondition::Vanish);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
			{
				LoopThroughConditions(clientId, Missions::CndSystemSpaceExit::SystemExitCondition::Dock);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall SystemSwitchOutComplete(unsigned int objId, unsigned int clientId)
			{
				LoopThroughConditions(clientId, Missions::CndSystemSpaceExit::SystemExitCondition::Jump);
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}