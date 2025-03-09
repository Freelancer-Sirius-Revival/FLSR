#pragma once
#include <memory>
#include <FLHook.h>
#include "Conditions.h"
#include "../MissionObject.h"

namespace Missions
{
	const uint Stranger = CreateID("stranger");

	struct ConditionParent
	{
		unsigned int missionId;
		unsigned int triggerId;
		ConditionParent(unsigned int missionId, unsigned int triggerId) : missionId(missionId), triggerId(triggerId) {}
	};

	struct Condition
	{
		const ConditionParent parent;
		const TriggerCondition type;
		MissionObject activator;

		Condition(const ConditionParent& parent, const TriggerCondition type);
		virtual ~Condition();
		virtual void Register();
		virtual void Unregister();
		virtual void ExecuteTrigger();
	};
	typedef std::shared_ptr<Condition> ConditionPtr;
}