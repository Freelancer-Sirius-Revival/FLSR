#include <FLHook.h>
#include "ActDestroy.h"

namespace Missions
{
	ActDestroy::ActDestroy(Trigger* parentTrigger, const ActDestroyArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_Destroy),
		objNameOrLabel(archetype->objNameOrLabel),
		destroyType(archetype->destroyType)
	{}

	void ActDestroy::Execute()
	{
		const std::wstring outputPretext = stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_Destroy " + stows(objNameOrLabel);
		// Copy list since the destruction of objects will in turn modify it via Destruction Hooks
		const auto originals = std::vector(trigger->mission->objects);
		for (const auto& object : originals)
		{
			if (object.name == objNameOrLabel || object.labels.contains(objNameOrLabel))
			{
				ConPrint(outputPretext + L"\n");
				if (pub::SpaceObj::ExistsAndAlive(object.id) == 0) //0 means alive, -2 dead
				{
					pub::SpaceObj::Destroy(object.id, destroyType);
				}
			}
		}
	}
}