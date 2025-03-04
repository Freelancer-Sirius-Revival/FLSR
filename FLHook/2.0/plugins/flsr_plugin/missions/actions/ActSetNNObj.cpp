#include <FLHook.h>
#include "ActSetNNObj.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActSetNNObj::ActSetNNObj(Trigger* parentTrigger, const ActSetNNObjArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_SetNNObj),
		archetype(actionArchetype)
	{}

	static void SetObjective(const uint clientId, const ActSetNNObj& action)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		if (action.archetype->message)
		{
			FmtStr caption(action.archetype->message, 0);
			caption.begin_mad_lib(action.archetype->message);
			caption.end_mad_lib();
			pub::Player::DisplayMissionMessage(clientId, caption, MissionMessageType::MissionMessageType_Type1, true);
		}

		if (action.archetype->systemId)
		{
			XRequestBestPath bestPath;
			bestPath.noPathFound = false;
			bestPath.repId = Players[clientId].iReputation;
			bestPath.waypointCount = 1;
			XRequestBestPathEntry target;
			target.systemId = action.archetype->systemId;
			target.position = action.archetype->position;
			const auto& object = action.trigger->mission->objectIdsByName.find(action.archetype->targetObjName);
			if (object != action.trigger->mission->objectIdsByName.end())
				target.objId = object->second;
			else
				target.objId = action.archetype->targetObjName;

			bestPath.entries[0] = target;
			if (action.archetype->bestRoute)
			{
				uint objId;
				pub::Player::GetShip(clientId, objId);
				if (!objId)
					pub::Player::GetBase(clientId, objId);

				IObjRW* inspect;
				StarSystem* starSystem;
				if (!objId || !GetShipInspect(objId, inspect, starSystem))
					return;

				bestPath.entries[1] = bestPath.entries[0];
				XRequestBestPathEntry start;
				start.position = inspect->cobj->vPos;
				start.systemId = inspect->cobj->system;
				start.objId = objId;
				bestPath.entries[0] = start;
				bestPath.waypointCount = 2;
				Server.RequestBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
			}
			else
			{
				pub::Player::ReturnBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
			}
		}
	}

	void ActSetNNObj::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_SetNNObj " + std::to_wstring(archetype->message) + L" for " + std::to_wstring(archetype->objNameOrLabel));
		FmtStr caption(archetype->message, 0);
		caption.begin_mad_lib(archetype->message);
		caption.end_mad_lib();
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger->condition->activator;
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				SetObjective(activator.id, *this);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Client)
					{
						SetObjective(object.id, *this);
						ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}