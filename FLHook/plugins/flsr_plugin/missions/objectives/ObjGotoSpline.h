#pragma once
#include "Objective.h"
#include "ObjGoto.h"

namespace Missions
{
	class ObjGotoSpline : public Objective, private ObjGoto
	{
	private:
		Vector spline[4];

	public:
		ObjGotoSpline(const ObjectiveParent& parent,
						const Vector spline[4],
						const bool noCruise,
						const float range,
						const float thrust,
						const uint objNameToWaitFor,
						const float startWaitDistance,
						const float endWaitDistance);
		void Execute(const ObjectiveState& state) const;
	};
}