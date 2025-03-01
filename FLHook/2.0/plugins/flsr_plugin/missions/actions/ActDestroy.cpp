#include "../../Main.h"
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
		for (auto it = trigger->mission->objects.begin(); it != trigger->mission->objects.end(); it++)
		{
			if (it->name == objNameOrLabel || it->labels.contains(objNameOrLabel))
			{
				ConPrint(outputPretext + L"\n");
				SolarSpawn::DestroySolar(it->id, destroyType);
			}
		}
	}
}