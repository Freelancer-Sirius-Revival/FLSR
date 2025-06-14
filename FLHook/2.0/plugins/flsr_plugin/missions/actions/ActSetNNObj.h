#pragma once
#include "Action.h"

namespace Missions
{
	enum class NNObjectiveType
	{
		Ids,
		Waypoint,
		Path,
		Objective
	};

	struct ActSetNNObj : Action
	{
		uint objNameOrLabel = 0;
		NNObjectiveType type = NNObjectiveType::Ids;
		uint titleId = 0;
		uint descriptionId = 0;
		uint systemId = 0;
		Vector position;
		uint targetObjName = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSetNNObj> ActSetNNObjPtr;
}