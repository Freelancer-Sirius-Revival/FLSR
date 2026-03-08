#include "CndCount.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndCount*> observedCndCount;
	std::unordered_map<uint, std::vector<CndCount*>> orderedCndCountByMissionId;

	CndCount::CndCount(const ConditionParent& parent, const uint label, const uint targetCount, const CountComparator comparator) :
		Condition(parent),
		label(label),
		targetCount(targetCount),
		comparator(comparator)
	{}

	CndCount::~CndCount()
	{
		Unregister();
	}

	void CndCount::Register()
	{
		if (observedCndCount.insert(this).second)
		{
			orderedCndCountByMissionId[parent.missionId].push_back(this);
			Hooks::CndCount::EvaluateCountConditions(parent.missionId, missions.at(parent.missionId).objectsByLabel, label);
		}
	}

	void CndCount::Unregister()
	{
		observedCndCount.erase(this);
		auto& orderedCndCount = orderedCndCountByMissionId[parent.missionId];
		if (const auto it = std::find(orderedCndCount.begin(), orderedCndCount.end(), this); it != orderedCndCount.end())
			orderedCndCount.erase(it);

		if (orderedCndCountByMissionId[parent.missionId].empty())
			orderedCndCountByMissionId.erase(parent.missionId);
	}

	bool CndCount::Matches(const uint currentLabel, const uint currentCount) const
	{
		if (currentLabel != label)
			return false;

		switch (comparator)
		{
			case CountComparator::Less:
				return currentCount < targetCount;

			case CountComparator::Equal:
				return currentCount == targetCount;

			case CountComparator::Greater:
				return currentCount > targetCount;

			default:
				return false;
		}
	}

	namespace Hooks
	{
		namespace CndCount
		{
			void EvaluateCountConditions(const uint missionId, const std::unordered_map<uint, std::vector<MissionObject>>& objectsByLabel, const uint label)
			{
				const auto& orderedCndCountEntry = orderedCndCountByMissionId.find(missionId);
				if (orderedCndCountEntry == orderedCndCountByMissionId.end() || orderedCndCountEntry->second.size() == 0)
					return;

				const auto& labelEntries = objectsByLabel.find(label);
				const uint count = labelEntries == objectsByLabel.end() ? 0 : labelEntries->second.size();

				const auto currentConditions(orderedCndCountEntry->second);
				for (const auto& condition : currentConditions)
				{
					if (observedCndCount.contains(condition) && condition->Matches(label, count))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}