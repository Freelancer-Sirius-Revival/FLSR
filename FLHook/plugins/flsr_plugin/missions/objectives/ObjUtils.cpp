#pragma once
#include "ObjUtils.h"

bool FindObjectByNameOrFirstPlayerByLabel(const Missions::Mission& mission, const uint targetObjNameOrPlayerLabel, uint& foundObjId)
{
	foundObjId = 0;
	if (const auto& objectEntry = mission.objectIdsByName.find(targetObjNameOrPlayerLabel); objectEntry != mission.objectIdsByName.end())
	{
		foundObjId = objectEntry->second;
	}
	else if (const auto& objectsByLabel = mission.objectsByLabel.find(targetObjNameOrPlayerLabel); objectsByLabel != mission.objectsByLabel.end())
	{
		for (const auto& object : objectsByLabel->second)
		{
			if (object.type == Missions::MissionObjectType::Client)
			{
				uint shipId = 0;
				pub::Player::GetShip(object.id, shipId);
				if (shipId)
				{
					foundObjId = shipId;
					ConPrint(L"Found player ship");
					break;
				}
			}
		}
	}
	if (pub::SpaceObj::ExistsAndAlive(foundObjId) != 0)
		foundObjId = 0;
	return foundObjId != 0;
}