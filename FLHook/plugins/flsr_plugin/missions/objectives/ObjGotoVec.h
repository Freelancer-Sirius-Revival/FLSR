#pragma once
#include "Objective.h"
#include "ObjGoto.h"

namespace Missions
{
	class ObjGotoVec : public Objective, private ObjGoto
	{
	private:
		const Vector position;

	public:
		ObjGotoVec(const ObjectiveParent& parent,
					const Vector& position,
					const bool noCruise,
					const float range,
					const float thrust,
					const uint objNameToWaitFor,
					const float startWaitDistance,
					const float endWaitDistance);
		void Execute(const ObjectiveState& state) const;
	};
}