#include <FLHook.h>
#include "ActAddLabel.h"
#include "../Mission.h"

namespace Missions
{
	ActAddLabel::ActAddLabel(const ActionParent& parent, const ActAddLabelArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_AddLabel),
		archetype(actionArchetype)
	{}

	void ActAddLabel::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_AddLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel) + L"\n");
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client)
			{
				mission.AddLabelToObject(activator, archetype->label);
			}
			else if (mission.objectIds.contains(activator.id))
			{
				mission.AddLabelToObject(activator, archetype->label);
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
				mission.AddLabelToObject(object, archetype->label);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					mission.AddLabelToObject(object, archetype->label);
			}
		}
	}
}