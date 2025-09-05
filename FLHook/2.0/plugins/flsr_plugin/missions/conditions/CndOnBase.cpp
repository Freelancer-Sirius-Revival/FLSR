#include "CndOnBase.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndOnBase*> observedCndOnBase;

	CndOnBase::CndOnBase(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& baseIds) :
		Condition(parent),
		label(label),
		baseIds(baseIds)
	{}

	CndOnBase::~CndOnBase()
	{
		Unregister();
	}

	void CndOnBase::Register()
	{
		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			if (Matches(playerData->iOnlineID, playerData->iBaseID))
			{
				ExecuteTrigger();
				return;
			}
		}
		observedCndOnBase.insert(this);
	}

	void CndOnBase::Unregister()
	{
		observedCndOnBase.erase(this);
	}

	bool CndOnBase::Matches(const uint clientId, const uint currentBaseId)
	{
		if (!currentBaseId)
			return false;

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
		namespace CndOnBase
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
			{
				returncode = DEFAULT_RETURNCODE;

				const std::unordered_set<Missions::CndOnBase*> currentConditions(observedCndOnBase);
				for (const auto& condition : currentConditions)
				{
					if (observedCndOnBase.contains(condition) && condition->Matches(clientId, baseId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}