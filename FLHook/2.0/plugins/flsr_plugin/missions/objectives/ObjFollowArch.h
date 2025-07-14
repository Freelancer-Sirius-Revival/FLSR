#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ObjFollowArchetype
	{
		uint objName = 0;
		float maxDistance = 100.0f;
		Vector relativePosition = { 0, 0, 0 };
		float unk = 400;
	};
	typedef std::shared_ptr<ObjFollowArchetype> ObjFollowArchetypePtr;
}