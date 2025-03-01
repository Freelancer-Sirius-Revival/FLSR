#include "ActLightFuse.h"
#include <FLHook.h>

namespace Missions
{
	ActLightFuse::ActLightFuse(Trigger* parentTrigger, const ActLightFuseArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_LightFuse),
		objNameOrLabel(archetype->objNameOrLabel),
		fuseId(archetype->fuseId)
	{}

	void ActLightFuse::Execute()
	{
		const std::wstring outputPretext = stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_LightFuse " + std::to_wstring(fuseId) + L" on " + stows(objNameOrLabel);
		for (auto it = trigger->mission->objects.begin(); it != trigger->mission->objects.end(); it++)
		{
			if (it->name == objNameOrLabel || it->labels.contains(objNameOrLabel))
			{
				IObjRW* inspect;
				StarSystem* starSystem;
				if (!GetShipInspect(it->id, inspect, starSystem))
					return;

				ConPrint(outputPretext + L"\n");
				HkLightFuse(inspect, fuseId, 0.0f, 0.0f, 0.0f);
			}
		}
	}
}