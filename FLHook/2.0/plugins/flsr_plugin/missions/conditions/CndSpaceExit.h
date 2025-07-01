#pragma once
#include "Condition.h"

namespace Missions
{
	class CndSpaceExit : public Condition
	{
	private:
		const uint objNameOrLabel;
		const uint systemId;

	public:
		CndSpaceExit(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId);
		~CndSpaceExit();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	extern std::unordered_set<CndSpaceExit*> spaceExitConditions;
}