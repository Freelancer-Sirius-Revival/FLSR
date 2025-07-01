#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndSpaceEnter : public Condition
	{
	private:
		const uint objNameOrLabel;
		const uint systemId;

	public:
		CndSpaceEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId);
		~CndSpaceEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	extern std::unordered_set<CndSpaceEnter*> spaceEnterConditions;
}