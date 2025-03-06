#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndTrue : Condition
	{
		CndTrue(const ConditionParent& parent);
		void Register();
	};
}