#include <FLHook.h>
#include "ActAddLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActAddLabel::ActAddLabel(Trigger* parentTrigger, const ActAddLabelArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_AddLabel),
		archetype(actionArchetype)
	{}

	static void AddLabelIfNotExisting(ActAddLabel& action, const MissionObject& object)
	{
		if (object.id == 0)
			return;

		bool found = false;
		for (const auto& objectByLabel : action.trigger->mission->objectsByLabel[action.archetype->label])
		{
			if (objectByLabel == object)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			action.trigger->mission->objectsByLabel[action.archetype->label].push_back(object);
			if (object.type == MissionObjectType::Client)
				ConPrint(L" client");
			else
				ConPrint(L" obj");
			ConPrint(L"[" + std::to_wstring(object.id) + L"]");
		}
	}

	void ActAddLabel::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_AddLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger->condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				// Clients are made known to the mission by giving them a label.
				trigger->mission->clientIds.insert(activator.id);
				AddLabelIfNotExisting(*this, activator);
			}
			else if (trigger->mission->objectIds.contains(activator.id))
			{
				AddLabelIfNotExisting(*this, activator);
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = trigger->mission->objectIdsByName.find(archetype->objNameOrLabel); objectByName != trigger->mission->objectIdsByName.end())
			{
				MissionObject object;
				object.type = MissionObjectType::Object;
				object.id = objectByName->second;
				AddLabelIfNotExisting(*this, object);
			}
			else if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					AddLabelIfNotExisting(*this, object);
			}
		}
		ConPrint(L"\n");
	}
}