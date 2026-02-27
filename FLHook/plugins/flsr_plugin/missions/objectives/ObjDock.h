#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjDock : public Objective
	{
	private:
		const uint targetObjNameOrId;

	public:
		ObjDock(const ObjectiveParent& parent, const uint targetObjNameOrId);
		void Execute(const ObjectiveState& state) const;
	};
}