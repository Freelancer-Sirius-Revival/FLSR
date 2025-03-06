#include <FLHook.h>
#include "ActDestroy.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActDestroy::ActDestroy(const ActionParent& parent, const ActDestroyArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_Destroy),
		archetype(actionArchetype)
	{}

	void ActDestroy::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_Destroy " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].condition->activator;
			uint objId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetShip(activator.id, objId);
			else
				objId = activator.id;

			if (objId && pub::SpaceObj::ExistsAndAlive(objId) == 0)
			{
				pub::SpaceObj::Destroy(objId, archetype->destroyType);
				if (activator.type == MissionObjectType::Client)
					ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
				else
					ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->objNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
			{
				const uint objId = objectByName->second;
				if (pub::SpaceObj::ExistsAndAlive(objId) == 0)
				{
					pub::SpaceObj::Destroy(objId, archetype->destroyType);
					ConPrint(L" obj[" + std::to_wstring(objId) + L"]");
				}
			}
			else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
			{
				// Copy list since the destruction of objects will in turn modify it via other hooks
				const std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
				{
					uint objId;
					if (object.type == MissionObjectType::Client)
						pub::Player::GetShip(object.id, objId);
					else
						objId = object.id;

					if (objId && pub::SpaceObj::ExistsAndAlive(objId) == 0)
					{
						pub::SpaceObj::Destroy(objId, archetype->destroyType);
						if (object.type == MissionObjectType::Client)
							ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
						else
							ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}