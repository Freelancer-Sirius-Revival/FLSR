#include <FLHook.h>
#include "ActAddLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActAddLabel::ActAddLabel(Trigger* parentTrigger, const ActAddLabelArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_AddLabel),
		archetype(actionArchetype)
	{}

	void ActAddLabel::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_AddLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == CreateID("activator"))
		{
			if (trigger->condition->activator.clientId)
			{
				bool entryFound = false;
				for (auto& object : trigger->mission->objects)
				{
					if (object.clientId == trigger->condition->activator.clientId)
					{
						object.labels.insert(archetype->label);
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
						entryFound = true;
						break;
					}
				}
				// Add player to the mission
				if (!entryFound)
				{
					MissionObject obj;
					obj.objId = trigger->condition->activator.objId;
					obj.name = CreateID("player");
					obj.labels.insert(archetype->label);
					obj.clientId = trigger->condition->activator.clientId;
					trigger->mission->objects.push_back(obj);
					ConPrint(L" client[" + std::to_wstring(obj.clientId) + L"]");
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.objId == trigger->condition->activator.objId)
					{
						object.labels.insert(archetype->label);
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
					object.labels.insert(archetype->label);
					if (object.clientId)
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
					else
						ConPrint(L" obj[" + std::to_wstring(object.objId) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}