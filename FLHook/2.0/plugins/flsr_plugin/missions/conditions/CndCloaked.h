#pragma once
#include "Condition.h"

namespace Missions
{
	class CndCloaked : public Condition
	{
	private:
		const uint objNameOrLabel;
		const bool cloaked;

	public:
		CndCloaked(const ConditionParent& parent, const uint objNameOrLabel, const bool cloaked);
		~CndCloaked();
		void Register();
		void Unregister();
		bool Matches(const MissionObject object);
	};

	extern std::unordered_set<CndCloaked*> cloakedConditions;
}