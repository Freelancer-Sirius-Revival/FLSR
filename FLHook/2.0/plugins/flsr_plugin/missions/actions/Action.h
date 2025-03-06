#pragma once
#include "../Trigger.h"

namespace Missions
{
	const uint Activator = CreateID("activator");

	struct ActionParent
	{
		unsigned int missionId;
		unsigned int triggerId;
	};

	struct Action
	{
		const ActionParent parent;
		const TriggerAction type;

		Action(const ActionParent& parent, const TriggerAction type) :
			parent(parent),
			type(type)
		{}
		virtual ~Action() {}
		virtual void Execute() = 0;
	};
	typedef std::shared_ptr<Action> ActionPtr;
}