#include <FLHook.h>
#include "ActAddLabel.h"

namespace Missions
{
	ActAddLabel::ActAddLabel(Trigger* parentTrigger, const ActAddLabelArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_AddLabel),
		objNameOrLabel(archetype->objNameOrLabel),
		label(archetype->label)
	{}

	void ActAddLabel::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_AddLabel " + stows(label) + L" to " + stows(objNameOrLabel) + L"\n");
		for (auto& entry : trigger->mission->objects)
		{
			if (entry.name == objNameOrLabel || entry.labels.contains(objNameOrLabel))
			{
				entry.labels.insert(label);
			}
		}
	}
}