#pragma once
#include "Condition.h"

namespace Missions
{
	class CndTrue : public Condition
	{
	public:
		CndTrue(const ConditionParent& parent);
		void Register();
	};
}