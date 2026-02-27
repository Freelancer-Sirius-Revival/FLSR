#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjFollow : public Objective
	{
	private:
		const uint targetObjName;
		const float maxDistance;
		const Vector position;

	public:
		ObjFollow(const ObjectiveParent& parent, const uint targetObjName, const float maxDistance, const Vector& position);
		void Execute(const ObjectiveState& state) const;
	};
}