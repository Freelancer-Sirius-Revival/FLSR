#include "ActSendComm.h"

namespace Missions
{
	static void SendComm(uint receiverObjId, uint senderObjId, const std::vector<uint>& lines, float delay, bool global)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (GetShipInspect(senderObjId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
		{
			const auto& object = dynamic_cast<CEqObj*>(inspect->cobj);
			pub::SpaceObj::SendComm(object->id, receiverObjId, object->voiceId, &object->commCostume, 0, (uint*)lines.data(), lines.size(), 0, std::fmaxf(0.0f, delay), global);
		}
	}

	void ActSendComm::Execute(Mission& mission, const MissionObject& activator) const
	{
		const auto& senderByName = mission.objectIdsByName.find(senderObjName);
		if (senderByName == mission.objectIdsByName.end())
			return;
		
		if (receiverObjNameOrLabel == Activator)
		{
			uint objId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetShip(activator.id, objId);
			else
				objId = activator.id;

			if (objId)
				SendComm(objId, senderByName->second, lines, delay, global);
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(receiverObjNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SendComm(objectByName->second, senderByName->second, lines, delay, global);
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
						SendComm(objId, senderByName->second, lines, delay, global);
				}
			}
		}
	}
}