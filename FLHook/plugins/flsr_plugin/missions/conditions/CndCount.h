#pragma once
#include "Condition.h"

namespace Missions
{
	class CndCount : public Condition
	{
	public:
		enum class CountComparator
		{
			Less,
			Equal,
			Greater
		};

	private:
		const uint label;
		const uint targetCount;
		const CountComparator comparator;

	public:
		CndCount(const ConditionParent& parent, const uint label, const uint targetCount, const CountComparator comparator);
		~CndCount();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint baseId) const;
	};

	namespace Hooks
	{
		namespace CndCount
		{
			void EvaluateCountConditions(const uint missionId, const std::unordered_map<uint, std::vector<MissionObject>>& objectsByLabel, const uint label);
		}
	}
}