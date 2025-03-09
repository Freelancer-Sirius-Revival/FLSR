#pragma once
#include <FLHook.h>

namespace Missions
{
	enum class GotoMovement
	{
		Cruise,
		NoCruise
	};

	struct ObjGotoArchetype
	{
		pub::AI::GotoOpType type = pub::AI::GotoOpType::Undefined;
		GotoMovement movement = GotoMovement::NoCruise;
		uint targetObjNameOrId = 0;
		Vector position = { 0, 0, 0 };
		Vector spline[4] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
		float range = 0.0f;
		float thrust = -1.0f;
		uint objNameToWaitFor = 0;
		float startWaitDistance = 0.0f;
		float endWaitDistance = 0.0f;
	};
	typedef std::shared_ptr<ObjGotoArchetype> ObjGotoArchetypePtr;
}