#include <FLHook.h>
#include "ActGiveObjList.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActGiveObjList::ActGiveObjList(const ActionParent& parent, const ActGiveObjListArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_GiveObjList),
		archetype(actionArchetype)
	{}

	static bool SetObjectiveList(const uint missionId, const uint objectivesId, uint objId)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return false;

		auto& mission = missions[missionId];
		if (const auto& objectivesEntry = mission.archetype->objectives.find(objectivesId); objectivesEntry != mission.archetype->objectives.end())
		{
			const Objectives objectives(missionId, objId, objectivesEntry->second->objectives);
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
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_GiveObjList " + std::to_wstring(archetype->objectivesId) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].condition->activator;
			if (activator.type == MissionObjectType::Object && missions[parent.missionId].objectIds.contains(activator.id) && SetObjectiveList(parent.missionId, archetype->objectivesId, activator.id))
				ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->objNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
			{
				if (SetObjectiveList(parent.missionId, archetype->objectivesId, objectByName->second))
					ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Object && SetObjectiveList(parent.missionId, archetype->objectivesId, object.id))
						ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}