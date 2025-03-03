#include <FLHook.h>
#include "ActLightFuse.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActLightFuse::ActLightFuse(Trigger* parentTrigger, const ActLightFuseArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_LightFuse),
		archetype(actionArchetype)
	{}

	static bool Fuse(const uint fuseId, uint objId)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return false;
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem))
			return false;
		HkLightFuse(inspect, fuseId, 0.0f, 0.0f, 0.0f);
		return true;
	}

	void ActLightFuse::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_LightFuse " + std::to_wstring(archetype->fuseId) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == CreateID("activator"))
		{
			auto& activator = trigger->condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				uint objId;
				pub::Player::GetShip(activator.id, objId);
				if (objId && Fuse(archetype->fuseId, objId))
					ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
			else if (trigger->mission->objectIds.contains(activator.id) && Fuse(archetype->fuseId, activator.id))
			{
				ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = trigger->mission->objectIdsByName.find(archetype->objNameOrLabel); objectByName != trigger->mission->objectIdsByName.end())
			{
				if (Fuse(archetype->fuseId, objectByName->second))
					ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (Fuse(archetype->fuseId, object.id))
					{
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