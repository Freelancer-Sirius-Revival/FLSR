#include "CndCount.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_map<uint, std::unordered_set<CndCount*>> countConditionsByMission;

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
		countConditionsByMission[parent.missionId].insert(this);
		missions.at(parent.missionId).EvaluateCountConditions(label);
	}

	void CndCount::Unregister()
	{
		countConditionsByMission[parent.missionId].erase(this);
	}

	bool CndCount::Matches(const uint currentLabel, const uint currentCount)
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
}