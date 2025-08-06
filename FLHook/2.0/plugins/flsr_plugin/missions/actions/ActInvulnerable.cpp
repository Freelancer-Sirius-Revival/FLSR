#include "ActInvulnerable.h"

namespace Missions
{
	void ActInvulnerable::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Client)
			{
				return; // This could interfer with Spawn Protection.
				//uint objId;
				//pub::Player::GetShip(activator.id, objId);
				//if (objId)
				//	pub::SpaceObj::SetInvincible2(objId, preventNonPlayerDamage, preventPlayerDamage, maxHpLossPercentage);
			}
			else if (mission.objectIds.contains(activator.id) && pub::SpaceObj::ExistsAndAlive(activator.id) == 0)
				pub::SpaceObj::SetInvincible2(activator.id, preventNonPlayerDamage, preventPlayerDamage, maxHpLossPercentage);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				const uint objId = objectByName->second;
				if (pub::SpaceObj::ExistsAndAlive(objId) == 0)
					pub::SpaceObj::SetInvincible2(objId, preventNonPlayerDamage, preventPlayerDamage, maxHpLossPercentage);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Client)
					{
						return; // This could interfer with Spawn Protection.
						//uint objId;
						//pub::Player::GetShip(activator.id, objId);
						//if (objId)
						//	pub::SpaceObj::SetInvincible2(objId, preventNonPlayerDamage, preventPlayerDamage, maxHpLossPercentage);
					}
					else if (pub::SpaceObj::ExistsAndAlive(object.id) == 0)
					{
						pub::SpaceObj::SetInvincible2(object.id, preventNonPlayerDamage, preventPlayerDamage, maxHpLossPercentage);
					}
				}
			}
		}
	}
}