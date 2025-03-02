#include <FLHook.h>
#include "ActDestroy.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActDestroy::ActDestroy(Trigger* parentTrigger, const ActDestroyArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_Destroy),
		objNameOrLabel(archetype->objNameOrLabel),
		destroyType(archetype->destroyType)
	{}

	void ActDestroy::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_Destroy " + stows(objNameOrLabel));
		if (objNameOrLabel == "activator")
		{
			if (trigger->condition->activator.clientId)
			{
				uint objId;
				pub::Player::GetShip(trigger->condition->activator.clientId, objId);
				if (objId && pub::SpaceObj::ExistsAndAlive(objId) == 0) //0 means alive, -2 dead
				{
					pub::SpaceObj::Destroy(objId, destroyType);
					ConPrint(L" client[" + std::to_wstring(trigger->condition->activator.clientId) + L"]");
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.id == trigger->condition->activator.objId)
					{
						if (pub::SpaceObj::ExistsAndAlive(object.id) == 0) //0 means alive, -2 dead
							pub::SpaceObj::Destroy(object.id, destroyType);
						ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
						break;
					}
				}
			}
		}
		else
		{
			// Copy list since the destruction of objects will in turn modify it via Destruction Hooks
			const auto originals = std::vector(trigger->mission->objects);
			for (const auto& object : originals)
			{
				if (object.name == objNameOrLabel || object.labels.contains(objNameOrLabel))
				{
					if (pub::SpaceObj::ExistsAndAlive(object.id) == 0) //0 means alive, -2 dead
						pub::SpaceObj::Destroy(object.id, destroyType);
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