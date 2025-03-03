#include <FLHook.h>
#include "ActRemoveLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActRemoveLabel::ActRemoveLabel(Trigger* parentTrigger, const ActRemoveLabelArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_RemoveLabel),
		archetype(actionArchetype)
	{}

	void ActRemoveLabel::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_RemoveLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == CreateID("activator"))
		{
			if (trigger->condition->activator.clientId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.clientId == trigger->condition->activator.clientId)
					{
						object.labels.erase(archetype->label);
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
						break;
					}
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.objId == trigger->condition->activator.objId)
					{
						object.labels.erase(archetype->label);
						ConPrint(L" obj[" + std::to_wstring(object.objId) + L"]");
						break;
					}
				}
			}
		}
		else
		{
			for (auto& object : trigger->mission->objects)
			{
				if (object.name == archetype->objNameOrLabel || object.labels.contains(archetype->objNameOrLabel))
				{
					object.labels.erase(archetype->label);
					if (object.clientId)
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
					else
						ConPrint(L" obj[" + std::to_wstring(object.objId) + L"]");
				}
			}
		}

		// Remove players without label from the mission
		for (auto it = trigger->mission->objects.begin(); it != trigger->mission->objects.end();)
		{
			if (it->clientId && it->labels.empty())
			{
				it = trigger->mission->objects.erase(it);
			}
			else
			{
				it++;
			}
		}
		ConPrint(L"\n");
	}
}