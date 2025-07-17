#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ObjDelay
	{
		uint timeInS;
	};
	typedef std::shared_ptr<ObjDelay> ObjDelayPtr;
}