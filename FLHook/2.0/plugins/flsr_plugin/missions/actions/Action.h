#pragma once
#include "ActionTypes.h"
#include <FLHook.h>

namespace Missions
{
	const uint Activator = CreateID("activator");

	struct ActionParent
	{
		const uint missionId;
		const uint triggerId;
		ActionParent(uint missionId, uint triggerId) : missionId(missionId), triggerId(triggerId) {}
	};

	struct Action
	{
		const ActionParent parent;
		const ActionType type;

		Action(const ActionParent& parent, const ActionType type) :
			parent(parent),
			type(type)
		{}
		virtual ~Action() {}
		virtual void Execute() = 0;
	};
}