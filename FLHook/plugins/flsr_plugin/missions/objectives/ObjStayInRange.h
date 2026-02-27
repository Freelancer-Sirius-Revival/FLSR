#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjStayInRange : public Objective
	{
	private:
		const uint targetObjNameOrId;
		const Vector position;
		const float range;
		const bool active;

	public:
		ObjStayInRange(const ObjectiveParent& parent, const uint targetObjNameOrId, const Vector position, const float range, const bool active);
		void Execute(const ObjectiveState& state) const;
	};
}