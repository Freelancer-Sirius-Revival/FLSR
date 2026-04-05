#pragma once
#include "Condition.h"
#include "../Mission.h"

namespace Missions
{
	Condition::Condition(const ConditionParent& parent) :
		parent(parent),
		activator(MissionObject(MissionObjectType::Client, 0))
	{}

	Condition::~Condition()
	{}

	ConditionPtr Condition::Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const
	{
		throw "Not Implemented";
	}

	void Condition::Register()
	{}

	void Condition::Unregister()
	{}

	void Condition::ExecuteTrigger()
	{
		missions.at(parent.missionId).QueueTriggerExecution(parent.triggerId, activator);
	}
}