#include "ObjSetLifeTime.h"
#include "ObjCndTrue.h"
#include "../ShipSpawning.h"

namespace Missions
{
	ObjSetLifeTime::ObjSetLifeTime(const ObjectiveParent& parent, const float lifetime) :
		Objective(parent),
		lifetime(lifetime)
	{}

	void ObjSetLifeTime::Execute(const ObjectiveState& state) const
	{
		ShipSpawning::SetLifeTime(state.objId, lifetime);
		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}