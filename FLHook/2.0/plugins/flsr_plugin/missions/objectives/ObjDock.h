#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ObjDock
	{
		uint targetObjNameOrId;
	};
	typedef std::shared_ptr<ObjDock> ObjDockPtr;
}