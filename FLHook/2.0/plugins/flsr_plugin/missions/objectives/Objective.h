#pragma once
#include <memory>
#include <FLHook.h>
#include "../Mission.h"
#include "../conditions/CndTrue.h"

namespace Missions
{
	struct ObjectiveParent
	{
		const uint missionId;
		const uint objectivesId;
		ObjectiveParent(const uint missionId, const uint objectivesId) : missionId(missionId), objectivesId(objectivesId) {}
	};

	class Objective
	{
	public:
		const ObjectiveParent parent;
		const int objectiveIndex;

		Objective(const ObjectiveParent& parent, const int objectiveIndex);
		virtual ~Objective() {};
		virtual void Execute(const uint objId) const;
	};
	typedef std::shared_ptr<Objective> ObjectivePtr;
}