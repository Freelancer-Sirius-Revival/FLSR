#include <FLHook.h>
#include "ActEtherComm.h"
#include "../Mission.h"

namespace Missions
{
	ActEtherComm::ActEtherComm(const ActionParent& parent, const ActEtherCommArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_EtherComm),
		archetype(actionArchetype)
	{}

	static void SendComm(uint receiverObjId, const ActEtherCommArchetypePtr archetype)
	{
		pub::SpaceObj::SendComm(0, receiverObjId, archetype->senderVoiceId, &archetype->costume, archetype->senderIdsName, archetype->lines.data(), archetype->lines.size(), 0, std::fmaxf(0.0f, archetype->delay), archetype->global);
	}

	void ActEtherComm::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_EtherComm to " + std::to_wstring(archetype->receiverObjNameOrLabel));
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
				SendComm(objId, archetype);
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
				SendComm(objectByName->second, archetype);
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
						SendComm(objId, archetype);
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