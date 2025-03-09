#pragma once
#include "Condition.h"
#include "../Trigger.h"

namespace Missions
{
	Condition::Condition(const ConditionParent& parent, const TriggerCondition type) :
		parent(parent),
		type(type)
	{}

	Condition::~Condition()
	{}

	void Condition::Register()
	{};

	void Condition::Unregister()
	{};

	void Condition::ExecuteTrigger()
	{
		triggers[parent.triggerId].QueueExecution();
	}
}