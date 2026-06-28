#pragma once
#include <memory>
#include <FLHook.h>
#include "../MissionObject.h"

namespace Missions
{
	const uint Stranger = CreateID("stranger");

	struct ConditionParent
	{
		const uint missionId;
		const uint triggerId;
		ConditionParent(const uint missionId, const uint triggerId) : missionId(missionId), triggerId(triggerId) {}
	};

	class Condition;
	typedef std::shared_ptr<Condition> ConditionPtr;

	class Condition
	{
	public:
		const ConditionParent parent;
		MissionObject activator;

		Condition(const ConditionParent& parent);
		virtual ~Condition();
		virtual ConditionPtr Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const;
		virtual void Register();
		virtual void Unregister();
		virtual void ExecuteTrigger();
	};
}