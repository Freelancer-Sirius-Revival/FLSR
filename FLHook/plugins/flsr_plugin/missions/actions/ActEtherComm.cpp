#include <FLHook.h>
#include "ActEtherComm.h"
#include "../Mission.h"

namespace Missions
{
	static void SendComm(const uint receiverObjId, const ActEtherComm& action, Mission& mission)
	{
		if (const auto& commEntry = mission.ongoingComms.find(action.id); commEntry != mission.ongoingComms.end())
		{
			commEntry->second.receiverObjIds.insert(receiverObjId);
		}
		else
		{
			Mission::CommEntry comm;
			comm.sendTime = timeInMS();
			comm.sender = MissionObject(MissionObjectType::Client, 0);
			comm.voiceLineId = action.lines.back();
			comm.receiverObjIds.insert(receiverObjId);
			mission.ongoingComms.insert({ action.id, comm });
		}

		pub::SpaceObj::SendComm(0, receiverObjId, action.senderVoiceId, &action.costume, action.senderIdsName, (uint*)(action.lines.data()), action.lines.size(), 0, std::fmaxf(0.0f, action.delay), action.global);
	}

	void ActEtherComm::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (receiverObjNameOrLabel == Activator)
		{
			uint objId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetShip(activator.id, objId);
			else
				objId = activator.id;

			if (objId)
				SendComm(objId, *this, mission);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(receiverObjNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SendComm(objectByName->second, *this, mission);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(receiverObjNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				// Copy list since the destruction of objects will in turn modify it via other hooks
				const std::vector<MissionObject> objectsCopy(objectsByLabel->second);
				for (const auto& object : objectsCopy)
				{
					uint objId;
					if (object.type == MissionObjectType::Client)
						pub::Player::GetShip(object.id, objId);
					else
						objId = object.id;

					if (objId)
						SendComm(objId, *this, mission);
				}
			}
		}
	}
}