#pragma once
#include "Condition.h"
#include "../Mission.h"

namespace Missions
{
	Condition::Condition(const ConditionParent& parent, const ConditionType type) :
		parent(parent),
		type(type),
		activator(MissionObject(MissionObjectType::Client, 0))
	{}

	Condition::~Condition()
	{}

	void Condition::Register()
	{};

	void Condition::Unregister()
	{};

	void Condition::ExecuteTrigger()
	{
		missions.at(parent.missionId).QueueTriggerExecution(parent.triggerId, activator);
	}
}