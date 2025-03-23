#include <FLHook.h>
#include "ActSendComm.h"
#include "../Mission.h"

namespace Missions
{
	ActSendComm::ActSendComm(const ActionParent& parent, const ActSendCommArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_SendComm),
		archetype(actionArchetype)
	{}

	static void SendComm(uint receiverObjId, uint senderObjId, std::vector<uint>& lines, float delay, bool global)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (GetShipInspect(senderObjId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
		{
			const auto& object = static_cast<CEqObj*>(inspect->cobj);
			pub::SpaceObj::SendComm(object->id, receiverObjId, object->voiceId, &object->commCostume, 0, lines.data(), lines.size(), 0, std::fmaxf(0.0f, delay), global);
		}
	}

	void ActSendComm::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_SendComm from " + std::to_wstring(archetype->senderObjName) + L" to " + std::to_wstring(archetype->receiverObjNameOrLabel));
		const auto& senderByName = mission.objectIdsByName.find(archetype->senderObjName);
		if (senderByName == mission.objectIdsByName.end())
			return;
		
		if (archetype->receiverObjNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			uint objId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetShip(activator.id, objId);
			else
				objId = activator.id;

			if (objId)
			{
				SendComm(objId, senderByName->second, archetype->lines, archetype->delay, archetype->global);
				if (activator.type == MissionObjectType::Client)
					ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
				else
					ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = mission.objectIdsByName.find(archetype->receiverObjNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SendComm(objectByName->second, senderByName->second, archetype->lines, archetype->delay, archetype->global);
				ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->receiverObjNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
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
					{
						SendComm(objId, senderByName->second, archetype->lines, archetype->delay, archetype->global);
						if (object.type == MissionObjectType::Client)
							ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
						else
							ConPrint(L" obj[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}