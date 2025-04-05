#include "CndCount.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_map<uint, std::unordered_set<CndCount*>> countConditionsByMission;

	CndCount::CndCount(const ConditionParent& parent, const CndCountArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_Count),
		archetype(conditionArchetype)
	{}

	CndCount::~CndCount()
	{
		Unregister();
	}

	void CndCount::Register()
	{
		countConditionsByMission[parent.missionId].insert(this);
		missions.at(parent.missionId).EvaluateCountConditions(archetype->label);
	}

	void CndCount::Unregister()
	{
		countConditionsByMission[parent.missionId].erase(this);
	}

	bool CndCount::Matches(const uint label, const uint count)
	{
		if (label != archetype->label)
			return false;

		bool matches = false;
		switch (archetype->comparator)
		{
			case CountComparator::Less:
				matches = count < archetype->count;
				break;

			case CountComparator::Equal:
				matches = count == archetype->count;
				break;

			case CountComparator::Greater:
				matches = count > archetype->count;
				break;
		}
		return matches;
	}
}