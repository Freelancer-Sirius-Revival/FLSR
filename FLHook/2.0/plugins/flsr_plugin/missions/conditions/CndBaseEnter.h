#pragma once
#include "Condition.h"

namespace Missions
{
	class CndBaseEnter : public Condition
	{
	private:
		const uint objNameOrLabel;
		const uint baseId;

	public:
		CndBaseEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint baseId);
		~CndBaseEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint baseId);
	};

	extern std::unordered_set<CndBaseEnter*> baseEnterConditions;
}