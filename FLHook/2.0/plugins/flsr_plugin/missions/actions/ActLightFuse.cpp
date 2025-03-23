#include <FLHook.h>
#include "ActLightFuse.h"
#include "../Mission.h"

namespace Missions
{
	ActLightFuse::ActLightFuse(const ActionParent& parent, const ActLightFuseArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_LightFuse),
		archetype(actionArchetype)
	{}

	static bool Fuse(const std::string& fuseName, uint objId)
	{
		if (fuseName.empty() || pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return false;
		pub::SpaceObj::LightFuse(objId, fuseName.c_str(), 0.0f);
		return true;
	}

	void ActLightFuse::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_LightFuse " + stows(archetype->fuseName) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client)
			{
				uint objId;
				pub::Player::GetShip(activator.id, objId);
				if (objId && Fuse(archetype->fuseName, objId))
					ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
			else if (mission.objectIds.contains(activator.id) && Fuse(archetype->fuseName, activator.id))
			{
				ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(archetype->objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				if (Fuse(archetype->fuseName, objectByName->second))
					ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (Fuse(archetype->fuseName, object.id))
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