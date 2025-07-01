#include "CndBaseEnter.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndBaseEnter*> baseEnterConditions;

	CndBaseEnter::CndBaseEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint baseId) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		baseId(baseId)
	{}

	CndBaseEnter::~CndBaseEnter()
	{
		Unregister();
	}

	void CndBaseEnter::Register()
	{
		baseEnterConditions.insert(this);
	}

	void CndBaseEnter::Unregister()
	{
		baseEnterConditions.erase(this);
	}

	bool CndBaseEnter::Matches(const uint clientId, const uint currentBaseId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (objNameOrLabel == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (!baseId || baseId == currentBaseId))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				return true;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
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
}