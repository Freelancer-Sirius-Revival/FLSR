#include "CndTrue.h"

namespace Missions
{
	CndTrue::CndTrue(const ConditionParent& parent) :
		Condition(parent)
	{}

	void CndTrue::Register()
	{
		ExecuteTrigger();
	}
}