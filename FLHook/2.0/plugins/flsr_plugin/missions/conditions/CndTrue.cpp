#include "CndTrue.h"

namespace Missions
{
	CndTrue::CndTrue(const ConditionParent& parent) :
		Condition(parent, TriggerCondition::Cnd_True)
	{}

	void CndTrue::Register()
	{
		triggers[parent.triggerId].QueueExecution();
	}
}