#include <FLHook.h>
#include "ActLightFuse.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActLightFuse::ActLightFuse(const ActionParent& parent, const ActLightFuseArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_LightFuse),
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
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_LightFuse " + std::to_wstring(archetype->fuseId) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			auto& activator = triggers[parent.triggerId].condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				uint objId;
				pub::Player::GetShip(activator.id, objId);
				if (objId && Fuse(archetype->fuseId, objId))
					ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
			else if (missions[parent.missionId].objectIds.contains(activator.id) && Fuse(archetype->fuseId, activator.id))
			{
				ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->objNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
			{
				if (Fuse(archetype->fuseId, objectByName->second))
					ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
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