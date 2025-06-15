#include "ActSetVibe.h"

namespace Missions
{
	static void SetVibe(const int reputationIdA, const uint targetIdOrLabel, const MissionObject& activator, const float reputation, const Mission& mission)
	{
		if (targetIdOrLabel == Activator)
		{
			int reputationIdB;
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
				int reputationIdB;
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
					int reputationIdB;
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
		if (objNameOrLabel == Activator)
		{
			int reputationId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetRep(activator.id, reputationId);
			else
				pub::SpaceObj::GetRep(activator.id, reputationId);

			if (reputationId)
				SetVibe(reputationId, targetObjNameOrLabel, activator, reputation, mission);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				int reputationId;
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
					int reputationId;
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