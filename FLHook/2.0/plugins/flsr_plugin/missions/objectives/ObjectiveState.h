#pragma once

struct ObjectiveState
{
	unsigned int objId;
	size_t objectiveIndex;
	bool enforceObjective;

	ObjectiveState(const unsigned int objId, const int objectiveIndex, const bool enforceObjective) :
		objId(objId),
		objectiveIndex(objectiveIndex),
		enforceObjective(enforceObjective)
	{}

	ObjectiveState(const ObjectiveState& state) :
		objId(state.objId),
		objectiveIndex(state.objectiveIndex),
		enforceObjective(state.enforceObjective)
	{}
};