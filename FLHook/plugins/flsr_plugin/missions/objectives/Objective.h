#pragma once
#include <memory>
#include <FLHook.h>
#include "ObjectiveState.h"
#include "../Mission.h"
#include "../conditions/Condition.h"

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
	protected:
		void RegisterCondition(const uint objId, const ConditionPtr& condition) const;
	public:
		const ObjectiveParent parent;

		Objective(const ObjectiveParent& parent);
		virtual ~Objective() {};
		virtual void Execute(const ObjectiveState& state) const;
	};
	typedef std::shared_ptr<Objective> ObjectivePtr;
}