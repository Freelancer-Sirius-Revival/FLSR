#include <FLHook.h>
#include "ActRemoveLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActRemoveLabel::ActRemoveLabel(const ActionParent& parent, const ActRemoveLabelArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_RemoveLabel),
		archetype(actionArchetype)
	{}

	static void RemoveLabel(ActRemoveLabel& action, const MissionObject& object)
	{
		if (object.id == 0)
			return;

		if (const auto& objectsByLabel = missions[action.parent.missionId].objectsByLabel.find(action.archetype->label); objectsByLabel != missions[action.parent.missionId].objectsByLabel.end())
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
				missions[action.parent.missionId].objectsByLabel.erase(action.archetype->label);
		}
	}

	static void UnregisterClient(ActRemoveLabel& action, const uint clientId)
	{
		bool clientFound = false;
		for (const auto& objectsByLabel : missions[action.parent.missionId].objectsByLabel)
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
			missions[action.parent.missionId].clientIds.erase(clientId);
	}

	void ActRemoveLabel::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_RemoveLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				RemoveLabel(*this, activator);
				// Clients without label should no longer be known to the mission.
				UnregisterClient(*this, activator.id);
			}
			else if (missions[parent.missionId].objectIds.contains(activator.id))
			{
				RemoveLabel(*this, activator);
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->objNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
			{
				MissionObject object;
				object.type = MissionObjectType::Object;
				object.id = objectByName->second;
				RemoveLabel(*this, object);
			}
			else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
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