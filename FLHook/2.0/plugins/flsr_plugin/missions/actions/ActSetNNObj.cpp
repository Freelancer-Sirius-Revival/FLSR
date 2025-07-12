#include "ActSetNNObj.h"

namespace Missions
{
	static void SetObjective(const Mission& mission, const uint clientId, const ActSetNNObj& action)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		if (action.message)
		{
			FmtStr caption(action.message, 0);
			caption.begin_mad_lib(action.message);
			caption.end_mad_lib();
			pub::Player::DisplayMissionMessage(clientId, caption, MissionMessageType::MissionMessageType_Type1, true);
		}

		XRequestBestPath bestPath;
		bestPath.noPathFound = false;
		bestPath.repId = Players[clientId].iReputation;
		if (action.systemId)
		{
			XRequestBestPathEntry target;
			target.systemId = action.systemId;
			target.position = action.position;
			const auto& object = mission.objectIdsByName.find(action.targetObjName);
			if (object != mission.objectIdsByName.end())
				target.objId = object->second;
			else
				target.objId = action.targetObjName;

			if (action.bestRoute)
			{
				uint objId;
				pub::Player::GetShip(clientId, objId);
				if (!objId)
					pub::Player::GetBase(clientId, objId);

				IObjRW* inspect;
				StarSystem* starSystem;
				if (!objId || !GetShipInspect(objId, inspect, starSystem))
					return;

				XRequestBestPathEntry start;
				start.position = inspect->cobj->vPos;
				start.systemId = inspect->cobj->system;
				start.objId = objId;

				bestPath.waypointCount = 2;
				bestPath.entries[0] = start;
				bestPath.entries[1] = target;
				Server.RequestBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
			}
			else
			{
				bestPath.waypointCount = 1;
				bestPath.entries[0] = target;
				pub::Player::ReturnBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
			}
		}
		// Clear waypoints
		else
		{
			bestPath.waypointCount = 0;
			pub::Player::ReturnBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
		}
	}

	void ActSetNNObj::Execute(Mission& mission, const MissionObject& activator) const
	{
		FmtStr caption(message, 0);
		caption.begin_mad_lib(message);
		caption.end_mad_lib();
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
				SetObjective(mission, activator.id, *this);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					SetObjective(mission, object.id, *this);
			}
		}
	}
}