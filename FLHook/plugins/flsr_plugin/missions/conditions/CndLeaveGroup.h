#pragma once
#include "Condition.h"

namespace Missions
{
	class CndLeaveGroup : public Condition
	{
	private:
		const uint label;

	public:
		CndLeaveGroup(const ConditionParent& parent, const uint label);
		~CndLeaveGroup();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, CPlayerGroup& group);
	};

	namespace Hooks
	{
		namespace CndLeaveGroup
		{
			bool __stdcall DelGroupMemberHook(CPlayerGroup* group, uint clientId);
		}
	}
}