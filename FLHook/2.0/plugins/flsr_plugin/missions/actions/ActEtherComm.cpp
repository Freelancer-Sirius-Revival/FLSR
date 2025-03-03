#include <FLHook.h>
#include "ActEtherComm.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActEtherComm::ActEtherComm(Trigger* parentTrigger, const ActEtherCommArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_EtherComm),
		archetype(actionArchetype)
	{}

	void ActEtherComm::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_EtherComm to " + std::to_wstring(archetype->receiverObjNameOrLabel));
		if (archetype->receiverObjNameOrLabel == Activator)
		{
			const auto& activator = trigger->condition->activator;
			uint objId;
			if (activator.type == MissionObjectType::Client)
				pub::Player::GetShip(activator.id, objId);
			else
				objId = activator.id;

			if (objId)
			{
				pub::SpaceObj::SendComm(0, objId, archetype->senderVoiceId, &archetype->costume, archetype->senderIdsName, archetype->lines.data(), archetype->lines.size(), 0, archetype->delay, archetype->global);
				if (activator.type == MissionObjectType::Client)
					ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
				else
					ConPrint(L" obj[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			// Clients can only be addressed via Label.
			if (const auto& objectByName = trigger->mission->objectIdsByName.find(archetype->receiverObjNameOrLabel); objectByName != trigger->mission->objectIdsByName.end())
			{
				pub::SpaceObj::SendComm(0, objectByName->second, archetype->senderVoiceId, &archetype->costume, archetype->senderIdsName, archetype->lines.data(), archetype->lines.size(), 0, archetype->delay, archetype->global);
				ConPrint(L" obj[" + std::to_wstring(objectByName->second) + L"]");
			}
			else if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->receiverObjNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
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
						pub::SpaceObj::SendComm(0, objId, archetype->senderVoiceId, &archetype->costume, archetype->senderIdsName, archetype->lines.data(), archetype->lines.size(), 0, archetype->delay, archetype->global);
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