#include <FLHook.h>
#include "ActRemoveLabel.h"
#include "../Mission.h"

namespace Missions
{
	ActRemoveLabel::ActRemoveLabel(const ActionParent& parent, const ActRemoveLabelArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_RemoveLabel),
		archetype(actionArchetype)
	{}

	static void RemoveLabel(ActRemoveLabel& action, const MissionObject& object)
	{
		if (object.id == 0)
			return;

		auto& mission = missions.at(action.parent.missionId);
		if (const auto& objectsByLabel = mission.objectsByLabel.find(action.archetype->label); objectsByLabel != mission.objectsByLabel.end())
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
				mission.objectsByLabel.erase(action.archetype->label);
		}
	}

	static void UnregisterClient(ActRemoveLabel& action, const uint clientId)
	{
		auto& mission = missions.at(action.parent.missionId);
		bool clientFound = false;
		for (const auto& objectsByLabel : mission.objectsByLabel)
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
			mission.clientIds.erase(clientId);
	}

	void ActRemoveLabel::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_RemoveLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client)
			{
				RemoveLabel(*this, activator);
				// Clients without label should no longer be known to the mission.
				UnregisterClient(*this, activator.id);
			}
			else if (mission.objectIds.contains(activator.id))
			{
				RemoveLabel(*this, activator);
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(archetype->objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				MissionObject object;
				object.type = MissionObjectType::Object;
				object.id = objectByName->second;
				RemoveLabel(*this, object);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
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