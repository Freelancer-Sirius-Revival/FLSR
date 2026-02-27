#include "ActSendComm.h"

namespace Missions
{
	static void SendComm(const uint receiverObjId, uint senderObjId, const ActSendComm& action, const MissionObject& senderObject, Mission& mission)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(senderObjId, inspect, starSystem) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return;

		if (const auto& commEntry = mission.ongoingComms.find(action.id); commEntry != mission.ongoingComms.end())
		{
			commEntry->second.receiverObjIds.insert(receiverObjId);
		}
		else
		{
			Mission::CommEntry comm;
			comm.sendTime = timeInMS();
			comm.sender = senderObject;
			comm.voiceLineId = action.lines.back();
			comm.receiverObjIds.insert(receiverObjId);
			mission.ongoingComms.insert({ action.id, comm });
		}

		const auto& object = dynamic_cast<CEqObj*>(inspect->cobj);
		pub::SpaceObj::SendComm(object->id, receiverObjId, object->voiceId, &object->commCostume, 0, (uint*)(action.lines.data()), action.lines.size(), 0, std::fmaxf(0.0f, action.delay), action.global);
	}

	void ActSendComm::Execute(Mission& mission, const MissionObject& activator) const
	{
		uint senderObjId = 0;
		// Try to find any solar in the entire game first.
		int reputationId;
		pub::SpaceObj::GetSolarRep(senderObjName, reputationId);
		// Solar IDs are the exact same as their reputation ID
		if (reputationId != 0 && senderObjName == reputationId)
			senderObjId = senderObjName;
		// Otherwise try to find the sender in the known objects of the mission.
		else if (const auto& senderByName = mission.objectIdsByName.find(senderObjName); senderByName != mission.objectIdsByName.end())
			senderObjId = senderByName->second;

		if (senderObjId == 0)
			return;

		uint receiverObjId = 0;
		pub::SpaceObj::GetSolarRep(receiverObjNameOrLabel, reputationId);
		// Solar IDs are the exact same as their reputation ID
		if (reputationId != 0 && receiverObjNameOrLabel == reputationId)
			SendComm(receiverObjId, senderObjId, *this, activator, mission);
		else if (receiverObjNameOrLabel == Activator)
		{
			uint objId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetShip(activator.id, objId);
			else
				objId = activator.id;

			if (objId)
				SendComm(objId, senderObjId, *this, activator, mission);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(receiverObjNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SendComm(objectByName->second, senderObjId, *this, MissionObject(MissionObjectType::Object, senderObjId), mission);
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
						SendComm(objId, senderObjId, *this, object, mission);
				}
			}
		}
	}
}