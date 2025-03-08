#include <FLHook.h>
#include "../../Pilots.h"
#include "ActSpawnShip.h"

namespace Missions
{
	ActSpawnShip::ActSpawnShip(const ActionParent& parent, const ActSpawnShipArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_SpawnShip),
		archetype(actionArchetype)
	{}

	static uint CreateNPC(const MsnNpcArchetype& msnNpc, const NpcArchetype& npc)
	{
		pub::SpaceObj::ShipInfo shipInfo;
		memset(&shipInfo, 0, sizeof(shipInfo));
		shipInfo.iFlag = 1;
		shipInfo.iSystem = msnNpc.systemId;
		shipInfo.iShipArchetype = npc.archetypeId;
		shipInfo.vPos = msnNpc.position;
		shipInfo.mOrientation = msnNpc.orientation;
		shipInfo.iLoadout = npc.loadoutId;
		shipInfo.Costume = npc.costume;
		shipInfo.iPilotVoice = npc.voiceId;
		shipInfo.iHitPointsLeft = -1;
		shipInfo.iLevel = npc.level;

		// Define the string used for the scanner name. Because the
		// following entry is empty, the pilot_name is used. This
		// can be overriden to display the ship type instead.
		FmtStr scanner_name(0, 0);
		scanner_name.begin_mad_lib(0);
		scanner_name.end_mad_lib();

		// Define the string used for the pilot name. The example
		// below shows the use of multiple part names.
		FmtStr pilot_name(0, 0);
		pilot_name.begin_mad_lib(16163); // ids of "%s0 %s1"
		pilot_name.end_mad_lib();

		pub::Reputation::Alloc(shipInfo.iRep, scanner_name, pilot_name);
		uint groupId;
		pub::Reputation::GetReputationGroup(groupId, npc.faction.c_str());
		pub::Reputation::SetAffiliation(shipInfo.iRep, groupId);

		uint objId;
		pub::SpaceObj::Create(objId, shipInfo);

		pub::AI::SetPersonalityParams personality;
		personality.personality = msnNpc.pilotJobId ? Pilots::GetPilotWithJob(npc.pilotId, msnNpc.pilotJobId) : Pilots::GetPilot(npc.pilotId);
		personality.state_graph = pub::StateGraph::get_state_graph(npc.stateGraph.c_str(), pub::StateGraph::TYPE_STANDARD);
		personality.state_id = true;
		pub::AI::SubmitState(objId, &personality);

		if (msnNpc.startingObjId)
		{
			uint launchObjId = msnNpc.startingObjId;
			IObjRW* inspect;
			StarSystem* starSystem;
			if (GetShipInspect(launchObjId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CSOLAR_OBJECT) == CObject::CSOLAR_OBJECT)
			{
				const auto solarArchetype = static_cast<Archetype::EqObj*>(inspect->cobj->archetype);
				if (solarArchetype->dockInfo.size() > 0)
					pub::SpaceObj::Launch(objId, msnNpc.startingObjId, 0);
			}
		}

		pub::AI::DirectiveCancelOp cancel;
		pub::AI::SubmitDirective(objId, &cancel);

		pub::AI::DirectiveIdleOp idle;
		idle.fireWeapons = true;
		pub::AI::SubmitDirective(objId, &idle);

		return objId;
	}

	void ActSpawnShip::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_SpawnShip " + stows(archetype->msnNpcName));
		if (missions[parent.missionId].objectIdsByName.contains(CreateID(archetype->msnNpcName.c_str())))
		{
			ConPrint(L" ALREADY EXISTS\n");
			return;
		}
		for (const auto& msnNpc : missions[parent.missionId].archetype->msnNpcs)
		{
			if (msnNpc->name == archetype->msnNpcName)
			{
				for (const auto& npc : missions[parent.missionId].archetype->npcs)
				{
					if (CreateID(npc->name.c_str()) == msnNpc->npcId)
					{
						const uint objId = CreateNPC(*msnNpc, *npc);
						if (objId)
						{
							missions[parent.missionId].objectIds.insert(objId);
							missions[parent.missionId].objectIdsByName[CreateID(msnNpc->name.c_str())] = objId;
							for (const auto& label : msnNpc->labels)
							{
								MissionObject object;
								object.type = MissionObjectType::Object;
								object.id = objId;
								missions[parent.missionId].objectsByLabel[label].push_back(object);
							}
							ConPrint(L" obj[" + std::to_wstring(objId) + L"]\n");
							return;
						}
						break;
					}
				}
				break;
			}
		}
		ConPrint(L" NOT FOUND\n");
	}
}