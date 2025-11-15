#include "../../Main.h"
#include "ActMark.h"

namespace Missions
{
	static void Mark(const uint clientId, const uint targetObjId, Mission& mission)
	{
		if (Mark::MarkObject(clientId, targetObjId))
			mission.markedObjIdsByClientId[clientId].insert(targetObjId);
	}

	static void Unmark(const uint clientId, const uint targetObjId, Mission& mission)
	{
		Mark::UnmarkObject(clientId, targetObjId);
		const auto& entry = mission.markedObjIdsByClientId.find(clientId);
		if (entry != mission.markedObjIdsByClientId.end())
		{
			entry->second.erase(targetObjId);
			if (entry->second.empty())
				mission.markedObjIdsByClientId.erase(entry);
		}
	}

	static void MarkObj(const uint clientId, const uint targetObjNameOrLabel, const bool mark, Mission& mission)
	{
		// Clients can only be addressed via Label.
		if (const auto& objectByName = mission.objectIdsByName.find(targetObjNameOrLabel); objectByName != mission.objectIdsByName.end())
		{
			if (mark)
				Mark(clientId, objectByName->second, mission);
			else
				Unmark(clientId, objectByName->second, mission);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(targetObjNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				uint objId;
				if (object.type == MissionObjectType::Client)
					pub::Player::GetShip(object.id, objId);
				else
					objId = object.id;
				if (mark)
					Mark(clientId, objId, mission);
				else
					Unmark(clientId, objId, mission);
			}
		}
		else
		{
			// Try to find any solar in the entire game first.
			int reputationId;
			pub::SpaceObj::GetSolarRep(targetObjNameOrLabel, reputationId);
			// Solar IDs are the exact same as their reputation ID
			if (reputationId != 0 && targetObjNameOrLabel == reputationId)
			{
				if (mark)
					Mark(clientId, targetObjNameOrLabel, mission);
				else
					Unmark(clientId, targetObjNameOrLabel, mission);
			}
		}
	}

	void ActMark::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				MarkObj(activator.id, targetObjNameOrLabel, marked, mission);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					MarkObj(object.id, targetObjNameOrLabel, marked, mission);
			}
		}
	}
}