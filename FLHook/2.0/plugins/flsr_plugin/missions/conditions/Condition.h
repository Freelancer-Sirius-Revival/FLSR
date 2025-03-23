#pragma once
#include <memory>
#include <FLHook.h>
#include "ConditionTypes.h"
#include "../MissionObject.h"

namespace Missions
{
	const uint Stranger = CreateID("stranger");

	struct ConditionParent
	{
		const uint missionId;
		const uint triggerId;
		ConditionParent(uint missionId, uint triggerId) : missionId(missionId), triggerId(triggerId) {}
	};

	struct Condition
	{
		const ConditionParent parent;
		const ConditionType type;

		Condition(const ConditionParent& parent, const ConditionType type);
		virtual ~Condition();
		virtual void Register();
		virtual void Unregister();
		virtual void ExecuteTrigger();
	};
	typedef std::shared_ptr<Condition> ConditionPtr;
}