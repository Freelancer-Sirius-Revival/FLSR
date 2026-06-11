#include "CndLeaveMsn.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndLeaveMsn*> observedCndLeaveMsn;
	std::unordered_map<uint, std::vector<CndLeaveMsn*>> orderedCndLeaveMsnByMissionId;

	CndLeaveMsn::CndLeaveMsn(const ConditionParent& parent, const uint label) :
		Condition(parent),
		label(label)
	{}

	CndLeaveMsn::~CndLeaveMsn()
	{
		Unregister();
	}

	void CndLeaveMsn::Register()
	{
		if (observedCndLeaveMsn.insert(this).second)
			orderedCndLeaveMsnByMissionId[parent.missionId].push_back(this);
	}

	void CndLeaveMsn::Unregister()
	{
		observedCndLeaveMsn.erase(this);
		auto& orderedCndLeaveMsn = orderedCndLeaveMsnByMissionId[parent.missionId];
		if (const auto it = std::find(orderedCndLeaveMsn.begin(), orderedCndLeaveMsn.end(), this); it != orderedCndLeaveMsn.end())
			orderedCndLeaveMsn.erase(it);

		if (orderedCndLeaveMsnByMissionId[parent.missionId].empty())
			orderedCndLeaveMsnByMissionId.erase(parent.missionId);
	}

	bool CndLeaveMsn::Matches(const uint clientId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == 0 && mission.clientIds.contains(clientId))
		{
			activator = MissionObject(MissionObjectType::Client, clientId);
			return true;
		}

		if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (labelObject.type == MissionObjectType::Client && clientId == labelObject.id)
				{
					activator = labelObject;
					return true;
				}
			}
			return false;
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndLeaveMsn
		{
			void EvaluateLeaveMission(const uint missionId, const uint clientId)
			{
				const auto& orderedCndLeaveMsnEntry = orderedCndLeaveMsnByMissionId.find(missionId);
				if (orderedCndLeaveMsnEntry == orderedCndLeaveMsnByMissionId.end() || orderedCndLeaveMsnEntry->second.size() == 0)
					return;

				const auto currentConditions(orderedCndLeaveMsnEntry->second);
				for (const auto& condition : currentConditions)
				{
					if (observedCndLeaveMsn.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}