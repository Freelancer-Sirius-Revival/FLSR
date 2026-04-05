#pragma once
#include "Condition.h"

namespace Missions
{
	class CndJoinGroup : public Condition
	{
	private:
		const uint label;

	public:
		CndJoinGroup(const ConditionParent& parent, const uint label);
		~CndJoinGroup();
		ConditionPtr Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const;
		void Register();
		void Unregister();
		bool Matches(const uint clientId, CPlayerGroup& group);
	};

	namespace Hooks
	{
		namespace CndJoinGroup
		{
			bool __stdcall AddGroupMemberHook_After(CPlayerGroup* group, uint clientId);
		}
	}
}