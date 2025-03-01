#include "CndTrue.h"

namespace Missions
{
	CndTrue::CndTrue(Trigger* parentTrigger) :
		Condition(parentTrigger, TriggerCondition::Cnd_True)
	{}

	void CndTrue::Register()
	{
		// Do nothing
	}

	void CndTrue::Unregister()
	{
		// Do nothing
	}
}