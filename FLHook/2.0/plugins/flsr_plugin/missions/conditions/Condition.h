#pragma once
#include "../Trigger.h"
#include "../MissionObject.h"

namespace Missions
{
	const uint Stranger = CreateID("stranger");

	struct ConditionParent
	{
		unsigned int missionId;
		unsigned int triggerId;
	};

	struct Condition
	{
		const ConditionParent parent;
		const TriggerCondition type;
		MissionObject activator;

		Condition(const ConditionParent& parent, const TriggerCondition type) :
			parent(parent),
			type(type)
		{}
		virtual ~Condition()
		{}
		virtual void Register() {};
		virtual void Unregister() {};
		void ExecuteTrigger()
		{
			triggers[parent.triggerId].QueueExecution();
		}
	};
	typedef std::shared_ptr<Condition> ConditionPtr;
}