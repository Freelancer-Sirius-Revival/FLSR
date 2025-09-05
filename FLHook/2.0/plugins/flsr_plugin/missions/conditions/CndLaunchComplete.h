#pragma once
#include "Condition.h"

namespace Missions
{
	class CndLaunchComplete : public Condition
	{
	private:
		const uint label;
		const std::unordered_set<uint> baseIds;

	public:
		CndLaunchComplete(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& baseIds);
		~CndLaunchComplete();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint currentBaseId);
	};

	namespace Hooks
	{
		namespace CndLaunchComplete
		{
			void __stdcall LaunchComplete_AFTER(unsigned int launchObjId, unsigned int shipId);
		}
	}
}