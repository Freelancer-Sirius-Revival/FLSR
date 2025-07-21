#pragma once

namespace Missions
{
	enum class GotoMovement
	{
		Cruise,
		NoCruise
	};

	struct ObjGoto
	{
		const bool noCruise;
		const float range;
		const float thrust;
		const unsigned int objNameToWaitFor;
		const float startWaitDistance;
		const float endWaitDistance;
		ObjGoto(const bool noCruise,
				const float range,
				const float thrust,
				const unsigned int objNameToWaitFor,
				const float startWaitDistance,
				const float endWaitDistance) :
			noCruise(noCruise),
			range(range),
			thrust(thrust),
			objNameToWaitFor(objNameToWaitFor),
			startWaitDistance(startWaitDistance),
			endWaitDistance(endWaitDistance)
		{}
	};
}