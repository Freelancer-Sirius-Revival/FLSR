#include <FLHook.h>
#include "ActGiveObjList.h"
#include "../Mission.h"

namespace Missions
{
	ActGiveObjList::ActGiveObjList(const ActionParent& parent, const ActGiveObjListArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_GiveObjList),
		archetype(actionArchetype)
	{}

	static bool SetObjectiveList(Mission& mission, const uint objectivesId, uint objId)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return false;

		if (const auto& objectivesEntry = mission.archetype->objectives.find(objectivesId); objectivesEntry != mission.archetype->objectives.end())
		{
			const Objectives objectives(mission.id, objId, objectivesEntry->second->objectives);
			if (mission.objectivesByObjectId.contains(objId))
				mission.objectivesByObjectId.erase(objId);
			mission.objectivesByObjectId.insert({ objId, objectives });
			mission.objectivesByObjectId[objId].Progress();
			return true;
		}
		return false;
	}

	void ActGiveObjList::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_GiveObjList " + std::to_wstring(archetype->objectivesId) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Object && mission.objectIds.contains(activator.id) && SetObjectiveList(mission, archetype->objectivesId, activator.id))
				ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(archetype->objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				if (SetObjectiveList(mission, archetype->objectivesId, objectByName->second))
					ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Object && SetObjectiveList(mission, archetype->objectivesId, object.id))
						ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}