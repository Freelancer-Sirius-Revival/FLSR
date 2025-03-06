#include <FLHook.h>
#include "ActAddLabel.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActAddLabel::ActAddLabel(const ActionParent& parent, const ActAddLabelArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_AddLabel),
		archetype(actionArchetype)
	{}

	static void AddLabelIfNotExisting(ActAddLabel& action, const MissionObject& object)
	{
		if (object.id == 0)
			return;

		if (object.type == MissionObjectType::Client && (!HkIsValidClientID(object.id) || HkIsInCharSelectMenu(object.id)))
			return;

		bool found = false;
		for (const auto& objectByLabel : missions[action.parent.missionId].objectsByLabel[action.archetype->label])
		{
			if (objectByLabel == object)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			missions[action.parent.missionId].objectsByLabel[action.archetype->label].push_back(object);
			if (object.type == MissionObjectType::Client)
				ConPrint(L" client");
			else
				ConPrint(L" obj");
			ConPrint(L"[" + std::to_wstring(object.id) + L"]");
		}
	}

	void ActAddLabel::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_AddLabel " + std::to_wstring(archetype->label) + L" to " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				// Clients are made known to the mission by giving them a label.
				missions[parent.missionId].clientIds.insert(activator.id);
				AddLabelIfNotExisting(*this, activator);
			}
			else if (missions[parent.missionId].objectIds.contains(activator.id))
			{
				AddLabelIfNotExisting(*this, activator);
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
				AddLabelIfNotExisting(*this, object);
			}
			else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					AddLabelIfNotExisting(*this, object);
			}
		}
		ConPrint(L"\n");
	}
}