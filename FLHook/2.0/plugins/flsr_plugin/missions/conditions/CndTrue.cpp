#include "CndTrue.h"

namespace Missions
{
	CndTrue::CndTrue(const ConditionParent& parent) :
		Condition(parent, ConditionType::Cnd_True)
	{}

	void CndTrue::Register()
	{
		ExecuteTrigger();
	}
}