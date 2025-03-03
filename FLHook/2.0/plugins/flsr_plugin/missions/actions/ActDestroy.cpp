#include <FLHook.h>
#include "ActDestroy.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActDestroy::ActDestroy(Trigger* parentTrigger, const ActDestroyArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_Destroy),
		archetype(actionArchetype)
	{}

	void ActDestroy::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_Destroy " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == CreateID("activator"))
		{
			if (trigger->condition->activator.clientId)
			{
				uint objId;
				pub::Player::GetShip(trigger->condition->activator.clientId, objId);
				if (objId && pub::SpaceObj::ExistsAndAlive(objId) == 0) //0 means alive, -2 dead
				{
					pub::SpaceObj::Destroy(objId, archetype->destroyType);
					ConPrint(L" client[" + std::to_wstring(trigger->condition->activator.clientId) + L"]");
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.objId == trigger->condition->activator.objId)
					{
						if (pub::SpaceObj::ExistsAndAlive(object.objId) == 0) //0 means alive, -2 dead
							pub::SpaceObj::Destroy(object.objId, archetype->destroyType);
						ConPrint(L" obj[" + std::to_wstring(object.objId) + L"]");
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
				if (object.name == archetype->objNameOrLabel || object.labels.contains(archetype->objNameOrLabel))
				{
					if (pub::SpaceObj::ExistsAndAlive(object.objId) == 0) //0 means alive, -2 dead
						pub::SpaceObj::Destroy(object.objId, archetype->destroyType);
					if (object.clientId)
						ConPrint(L" client[" + std::to_wstring(object.clientId) + L"]");
					else
						ConPrint(L" obj[" + std::to_wstring(object.objId) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}