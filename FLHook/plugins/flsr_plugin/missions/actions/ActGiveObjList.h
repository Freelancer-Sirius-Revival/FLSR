#pragma once
#include "Action.h"

namespace Missions
{
	struct ActGiveObjList : Action
	{
		uint objNameOrLabel = 0;
		uint objectivesId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActGiveObjList> ActGiveObjListPtr;
}