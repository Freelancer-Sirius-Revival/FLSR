#include "ActGiveObjList.h"

namespace Missions
{
	static void SetObjectiveList(Mission& mission, const uint objectivesId, uint objId)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		if (const auto& objectivesEntry = mission.archetype->objectives.find(objectivesId); objectivesEntry != mission.archetype->objectives.end())
		{
			const Objectives objectives(mission.id, objId, objectivesEntry->second->objectives);
			if (mission.objectivesByObjectId.contains(objId))
				mission.objectivesByObjectId.erase(objId);
			mission.objectivesByObjectId.insert({ objId, objectives });
			mission.objectivesByObjectId[objId].Progress();
			return;
		}
		return;
	}

	void ActGiveObjList::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Object && mission.objectIds.contains(activator.id))
				SetObjectiveList(mission, objectivesId, activator.id);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SetObjectiveList(mission, objectivesId, objectByName->second);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Object)
						SetObjectiveList(mission, objectivesId, object.id);
				}
			}
		}
	}
}