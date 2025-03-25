#include <FLHook.h>
#include "ActRemoveLabel.h"
#include "../Mission.h"

namespace Missions
{
	ActRemoveLabel::ActRemoveLabel(const ActionParent& parent, const ActRemoveLabelArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_RemoveLabel),
		archetype(actionArchetype)
	{}

	void ActRemoveLabel::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_RemoveLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel) + L"\n");
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client)
			{
				mission.RemoveLabelFromObject(activator, archetype->label);
			}
			else if (mission.objectIds.contains(activator.id))
			{
				mission.RemoveLabelFromObject(activator, archetype->label);
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
				mission.RemoveLabelFromObject(object, archetype->label);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				const std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
					mission.RemoveLabelFromObject(object, archetype->label);
			}
		}
	}
}