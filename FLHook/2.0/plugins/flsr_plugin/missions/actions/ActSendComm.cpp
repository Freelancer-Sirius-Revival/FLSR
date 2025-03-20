#include <FLHook.h>
#include "ActSendComm.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActSendComm::ActSendComm(const ActionParent& parent, const ActSendCommArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_SendComm),
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
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_SendComm from " + std::to_wstring(archetype->senderObjName) + L" to " + std::to_wstring(archetype->receiverObjNameOrLabel));
		const auto& senderByName = missions[parent.missionId].objectIdsByName.find(archetype->senderObjName);
		if (senderByName == missions[parent.missionId].objectIdsByName.end())
			return;
		
		if (archetype->receiverObjNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].activator;
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
			if (const auto& objectByName = missions[parent.missionId].objectIdsByName.find(archetype->receiverObjNameOrLabel); objectByName != missions[parent.missionId].objectIdsByName.end())
			{
				SendComm(objectByName->second, senderByName->second, archetype->lines, archetype->delay, archetype->global);
				ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->receiverObjNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
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