#include <FLHook.h>
#include "ActRemoveLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActRemoveLabel::ActRemoveLabel(Trigger* parentTrigger, const ActRemoveLabelArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_RemoveLabel),
		archetype(actionArchetype)
	{}

	static void RemoveLabel(ActRemoveLabel& action, const MissionObject& object)
	{
		if (object.id == 0)
			return;

		if (const auto& objectsByLabel = action.trigger->mission->objectsByLabel.find(action.archetype->label); objectsByLabel != action.trigger->mission->objectsByLabel.end())
		{
			for (auto it = objectsByLabel->second.begin(); it != objectsByLabel->second.end();)
			{
				if (*it == object)
				{
					if (it->type == MissionObjectType::Client)
						ConPrint(L" client");
					else
						ConPrint(L" obj");
					ConPrint(L"[" + std::to_wstring(it->id) + L"]");
					it = objectsByLabel->second.erase(it);
				}
				else
					it++;
			}
			if (objectsByLabel->second.empty())
				action.trigger->mission->objectsByLabel.erase(action.archetype->label);
		}
	}

	static void UnregisterClient(ActRemoveLabel& action, const uint clientId)
	{
		bool clientFound = false;
		for (const auto& objectsByLabel : action.trigger->mission->objectsByLabel)
		{
			for (const auto& object : objectsByLabel.second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId)
				{
					clientFound = true;
					break;
				}
			}
		}
		if (!clientFound)
			action.trigger->mission->clientIds.erase(clientId);
	}

	void ActRemoveLabel::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_RemoveLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger->condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				RemoveLabel(*this, activator);
				// Clients without label should no longer be known to the mission.
				UnregisterClient(*this, activator.id);
			}
			else if (trigger->mission->objectIds.contains(activator.id))
			{
				RemoveLabel(*this, activator);
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
				RemoveLabel(*this, object);
			}
			else if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
			{
				std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
				{
					RemoveLabel(*this, object);
				}
				// Clients without label should no longer be known to the mission.
				for (const auto& object : objectsCopy)
				{
					if (object.type == MissionObjectType::Client)
						UnregisterClient(*this, object.id);
				}
			}
		}
		ConPrint(L"\n");
	}
}