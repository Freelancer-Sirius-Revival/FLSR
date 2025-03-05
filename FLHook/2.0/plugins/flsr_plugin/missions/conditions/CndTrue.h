#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndTrue : Condition
	{
		CndTrue(Trigger* parentTrigger);
		void Register();
	};
}