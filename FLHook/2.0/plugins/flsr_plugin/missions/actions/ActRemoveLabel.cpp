#include <FLHook.h>
#include "ActRemoveLabel.h"

namespace Missions
{
	ActRemoveLabel::ActRemoveLabel(Trigger* parentTrigger, const ActRemoveLabelArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_RemoveLabel),
		objNameOrLabel(archetype->objNameOrLabel),
		label(archetype->label)
	{}

	void ActRemoveLabel::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_RemoveLabel " + stows(label) + L" to " + stows(objNameOrLabel) + L"\n");
		for (auto& entry : trigger->mission->objects)
		{
			if (entry.name == objNameOrLabel || entry.labels.contains(objNameOrLabel))
			{
				entry.labels.erase(label);
			}
		}
	}
}