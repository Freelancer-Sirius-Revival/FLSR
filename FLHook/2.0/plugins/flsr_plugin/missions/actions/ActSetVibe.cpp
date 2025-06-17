#include "ActSetVibe.h"

namespace Missions
{
	static void SetVibe(const int reputationIdA, const uint targetIdOrLabel, const MissionObject& activator, const float reputation, const Mission& mission)
	{
		int reputationIdB;
		// Try to find any solar in the entire game first.
		pub::SpaceObj::GetSolarRep(targetIdOrLabel, reputationIdB);
		// Solar IDs are the exact same as their reputation ID
		if (reputationIdB != 0 && targetIdOrLabel == reputationIdB)
			pub::Reputation::SetAttitude(reputationIdA, reputationIdB, reputation);
		else if (targetIdOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetRep(activator.id, reputationIdB);
			else
				pub::SpaceObj::GetRep(activator.id, reputationIdB);

			if (reputationIdB)
				pub::Reputation::SetAttitude(reputationIdA, reputationIdB, reputation);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(targetIdOrLabel); objectByName != mission.objectIdsByName.end())
			{
				pub::SpaceObj::GetRep(objectByName->second, reputationIdB);
				if (reputationIdB)
					pub::Reputation::SetAttitude(reputationIdA, reputationIdB, reputation);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(targetIdOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				// Copy list since the destruction of objects will in turn modify it via other hooks
				const std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
				{
					if (object.type == MissionObjectType::Client)
						pub::Player::GetRep(object.id, reputationIdB);
					else
						pub::SpaceObj::GetRep(object.id, reputationIdB);

					if (reputationIdB)
						pub::Reputation::SetAttitude(reputationIdA, reputationIdB, reputation);
				}
			}
		}
	}

	void ActSetVibe::Execute(Mission& mission, const MissionObject& activator) const
	{
		int reputationId;
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetRep(activator.id, reputationId);
			else
				pub::SpaceObj::GetRep(activator.id, reputationId);

			if (reputationId)
				SetVibe(reputationId, targetObjNameOrLabel, activator, reputation, mission);
		}
		else
		{
			// Try to find any solar in the entire game first.
			pub::SpaceObj::GetSolarRep(objNameOrLabel, reputationId);
			// Solar IDs are the exact same as their reputation ID
			if (reputationId != 0 && objNameOrLabel == reputationId)
				SetVibe(reputationId, targetObjNameOrLabel, activator, reputation, mission);
			// Clients can only be addressed via Label.
			else if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				pub::SpaceObj::GetRep(objectByName->second, reputationId);
				if (reputationId)
					SetVibe(reputationId, targetObjNameOrLabel, activator, reputation, mission);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				// Copy list since the destruction of objects will in turn modify it via other hooks
				const std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
				{
					if (object.type == MissionObjectType::Client)
						pub::Player::GetRep(object.id, reputationId);
					else
						pub::SpaceObj::GetRep(object.id, reputationId);

					if (reputationId)
						SetVibe(reputationId, targetObjNameOrLabel, activator, reputation, mission);
				}
			}
		}
	}
}