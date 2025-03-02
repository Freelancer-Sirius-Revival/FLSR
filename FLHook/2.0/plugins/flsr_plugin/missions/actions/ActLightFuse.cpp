#include <FLHook.h>
#include "ActLightFuse.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActLightFuse::ActLightFuse(Trigger* parentTrigger, const ActLightFuseArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_LightFuse),
		objNameOrLabel(archetype->objNameOrLabel),
		fuseId(archetype->fuseId)
	{}

	void ActLightFuse::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_LightFuse " + std::to_wstring(fuseId) + L" on " + stows(objNameOrLabel));
		if (objNameOrLabel == "activator")
		{
			if (trigger->condition->activator.clientId)
			{
				uint objId;
				pub::Player::GetShip(trigger->condition->activator.clientId, objId);
				if (objId)
				{
					IObjRW* inspect;
					StarSystem* starSystem;
					if (GetShipInspect(objId, inspect, starSystem))
					{
						HkLightFuse(inspect, fuseId, 0.0f, 0.0f, 0.0f);
						ConPrint(L" client[" + std::to_wstring(trigger->condition->activator.clientId) + L"]");
					}
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.id == trigger->condition->activator.objId)
					{
						IObjRW* inspect;
						StarSystem* starSystem;
						if (!GetShipInspect(object.id, inspect, starSystem))
							break;
						HkLightFuse(inspect, fuseId, 0.0f, 0.0f, 0.0f);
						ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
						break;
					}
				}
			}
		}
		else
		{
			for (auto it = trigger->mission->objects.begin(); it != trigger->mission->objects.end(); it++)
			{
				if (it->name == objNameOrLabel || it->labels.contains(objNameOrLabel))
				{
					IObjRW* inspect;
					StarSystem* starSystem;
					if (!GetShipInspect(it->id, inspect, starSystem))
						continue;
					HkLightFuse(inspect, fuseId, 0.0f, 0.0f, 0.0f);
					if (it->clientId)
						ConPrint(L" client[" + std::to_wstring(it->clientId) + L"]");
					else
						ConPrint(L" obj[" + std::to_wstring(it->id) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}