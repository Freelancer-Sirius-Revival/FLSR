#pragma once
#include "Objective.h"
#include "ObjGoto.h"

namespace Missions
{
	class ObjGotoObj : public Objective, private ObjGoto
	{
	private:
		const uint targetObjNameOrId;

	public:
		ObjGotoObj(const ObjectiveParent& parent,
					const uint targetObjNameOrId,
					const bool noCruise,
					const float range,
					const float thrust,
					const uint objNameToWaitFor,
					const float startWaitDistance,
					const float endWaitDistance);
		void Execute(const ObjectiveState& state) const;
	};
}