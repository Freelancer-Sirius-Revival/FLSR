#include "CndCount.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_map<uint, std::unordered_set<CndCount*>> registeredConditionsByMissionId;

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
		registeredConditionsByMissionId[parent.missionId].insert(this);
		Hooks::CndCount::EvaluateCountConditions(parent.missionId, missions.at(parent.missionId).objectsByLabel, label);
	}

	void CndCount::Unregister()
	{
		registeredConditionsByMissionId[parent.missionId].erase(this);
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
				const auto& conditionsEntry = registeredConditionsByMissionId.find(missionId);
				if (conditionsEntry == registeredConditionsByMissionId.end() || conditionsEntry->second.size() == 0)
					return;

				const auto& registeredConditions = conditionsEntry->second;
				const auto& labelEntries = objectsByLabel.find(label);
				const uint count = labelEntries == objectsByLabel.end() ? 0 : labelEntries->second.size();

				const std::unordered_set<Missions::CndCount*> currentConditions(registeredConditions);
				for (const auto& condition : currentConditions)
				{
					if (registeredConditions.contains(condition) && condition->Matches(label, count))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}