#include <FLHook.h>
#include "ActAddLabel.h"
#include "../Mission.h"

namespace Missions
{
	ActAddLabel::ActAddLabel(const ActionParent& parent, const ActAddLabelArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_AddLabel),
		archetype(actionArchetype)
	{}

	static void AddLabelIfNotExisting(ActAddLabel& action, const MissionObject& object)
	{
		if (object.id == 0)
			return;

		if (object.type == MissionObjectType::Client && (!HkIsValidClientID(object.id) || HkIsInCharSelectMenu(object.id)))
			return;

		auto& mission = missions.at(action.parent.missionId);
		bool found = false;
		for (const auto& objectByLabel : mission.objectsByLabel[action.archetype->label])
		{
			if (objectByLabel == object)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			mission.objectsByLabel[action.archetype->label].push_back(object);
			if (object.type == MissionObjectType::Client)
				ConPrint(L" client");
			else
				ConPrint(L" obj");
			ConPrint(L"[" + std::to_wstring(object.id) + L"]");
		}
	}

	void ActAddLabel::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_AddLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client)
			{
				// Clients are made known to the mission by giving them a label.
				mission.clientIds.insert(activator.id);
				AddLabelIfNotExisting(*this, activator);
			}
			else if (mission.objectIds.contains(activator.id))
			{
				AddLabelIfNotExisting(*this, activator);
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
				AddLabelIfNotExisting(*this, object);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					AddLabelIfNotExisting(*this, object);
			}
		}
		ConPrint(L"\n");
	}
}