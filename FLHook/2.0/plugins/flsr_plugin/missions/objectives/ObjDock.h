#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjDock : public Objective
	{
	private:
		const uint targetObjNameOrId;

	public:
		ObjDock(const ObjectiveParent& parent, const int objectiveIndex, const uint targetObjNameOrId);
		void Execute(const uint objId) const;
	};
}