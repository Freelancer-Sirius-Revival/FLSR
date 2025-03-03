#include <FLHook.h>
#include "ActLightFuse.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActLightFuse::ActLightFuse(Trigger* parentTrigger, const ActLightFuseArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_LightFuse),
		archetype(actionArchetype)
	{}

	void ActLightFuse::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_LightFuse " + std::to_wstring(archetype->fuseId) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == CreateID("activator"))
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
						HkLightFuse(inspect, archetype->fuseId, 0.0f, 0.0f, 0.0f);
						ConPrint(L" client[" + std::to_wstring(trigger->condition->activator.clientId) + L"]");
					}
				}
			}
			else if (trigger->condition->activator.objId)
			{
				for (auto& object : trigger->mission->objects)
				{
					if (object.objId == trigger->condition->activator.objId)
					{
						IObjRW* inspect;
						StarSystem* starSystem;
						if (!GetShipInspect(object.objId, inspect, starSystem))
							break;
						HkLightFuse(inspect, archetype->fuseId, 0.0f, 0.0f, 0.0f);
						ConPrint(L" obj[" + std::to_wstring(object.objId) + L"]");
						break;
					}
				}
			}
		}
		else
		{
			for (auto it = trigger->mission->objects.begin(); it != trigger->mission->objects.end(); it++)
			{
				if (it->name == archetype->objNameOrLabel || it->labels.contains(archetype->objNameOrLabel))
				{
					IObjRW* inspect;
					StarSystem* starSystem;
					if (!GetShipInspect(it->objId, inspect, starSystem))
						continue;
					HkLightFuse(inspect, archetype->fuseId, 0.0f, 0.0f, 0.0f);
					if (it->clientId)
						ConPrint(L" client[" + std::to_wstring(it->clientId) + L"]");
					else
						ConPrint(L" obj[" + std::to_wstring(it->objId) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}