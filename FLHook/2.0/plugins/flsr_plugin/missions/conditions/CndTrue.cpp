#include "CndTrue.h"

namespace Missions
{
	CndTrue::CndTrue(Trigger* parentTrigger) :
		Condition(parentTrigger, TriggerCondition::Cnd_True)
	{}

	void CndTrue::Register()
	{
		trigger->QueueExecution();
	}
}