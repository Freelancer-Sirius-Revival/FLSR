#include <FLHook.h>
#include "ActRemoveLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActRemoveLabel::ActRemoveLabel(Trigger* parentTrigger, const ActRemoveLabelArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_RemoveLabel),
		objNameOrLabel(archetype->objNameOrLabel),
		label(archetype->label)
	{}

	void ActRemoveLabel::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_RemoveLabel " + stows(label) + L" to " + stows(objNameOrLabel));
		if (objNameOrLabel == "activator")
		{
			if (trigger->condition->activator.clientId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.clientId == trigger->condition->activator.clientId)
					{
						object.labels.erase(label);
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
						break;
					}
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.id == trigger->condition->activator.objId)
					{
						object.labels.erase(label);
						ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
						break;
					}
				}
			}
		}
		else
		{
			for (auto& object : trigger->mission->objects)
			{
				if (object.name == objNameOrLabel || object.labels.contains(objNameOrLabel))
				{
					object.labels.erase(label);
					if (object.clientId)
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
					else
						ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}