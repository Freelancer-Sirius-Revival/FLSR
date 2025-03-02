#include <FLHook.h>
#include "ActAddLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActAddLabel::ActAddLabel(Trigger* parentTrigger, const ActAddLabelArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_AddLabel),
		objNameOrLabel(archetype->objNameOrLabel),
		label(archetype->label)
	{}

	void ActAddLabel::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_AddLabel " + stows(label) + L" to " + stows(objNameOrLabel));
		if (objNameOrLabel == "activator")
		{
			if (trigger->condition->activator.clientId)
			{
				bool entryFound = false;
				for (auto& object : trigger->mission->objects)
				{
					if (object.clientId == trigger->condition->activator.clientId)
					{
						object.labels.insert(label);
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
						entryFound = true;
						break;
					}
				}
				if (!entryFound)
				{
					MissionObject obj;
					obj.id = trigger->condition->activator.objId;
					obj.name = "player";
					obj.labels.insert(label);
					obj.clientId = trigger->condition->activator.clientId;
					trigger->mission->objects.push_back(obj);
					ConPrint(L" client[" + std::to_wstring(obj.clientId) + L"]");
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.id == trigger->condition->activator.objId)
					{
						object.labels.insert(label);
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
					object.labels.insert(label);
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