#include <FLHook.h>
#include "../../Pilots.h"
#include "../NpcNames.h"
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

		// Formation name is displayed above the pilot name in wireframe display.
		// If this is given, the ship IDS Name will be used in scanner list. Keep empty to show Pilot Name in scanner list.
		FmtStr formationName(0, 0);
		formationName.begin_mad_lib(0);
		formationName.end_mad_lib();

		// Pilot name to be displayed when clicking on the ship/wireframe display.
		// Will be also displayed in scanner list if no formation name is given.
		FmtStr pilotName(0, 0);
		pilotName.begin_mad_lib(0);
		pilotName.end_mad_lib();

		if (msnNpc.idsName)
		{
			pilotName.begin_mad_lib(msnNpc.idsName);
			pilotName.end_mad_lib();
		}
		else
		{
			const auto& ship = Archetype::GetShip(npc.archetypeId);
			bool largeShip = ship->iArchType & (ObjectType::Gunboat | ObjectType::Cruiser | ObjectType::Transport | ObjectType::Capital | ObjectType::Mining);

			if (!npc.faction.empty())
			{
				if (largeShip)
				{
					const auto& result = NpcNames::GetRandomLargeShipName(CreateID(npc.faction.c_str()));
					pilotName.begin_mad_lib(16162); // "%s0 %s1 %s2"
					pilotName.end_mad_lib();
					pilotName.append_string(ship->iIdsName);
					pilotName.append_string(result.first);
					pilotName.append_string(result.second);
				}
				else if (npc.voiceId)
				{
					pilotName.begin_mad_lib(16163); // "%s0 %s1"
					pilotName.end_mad_lib();
					const auto& result = NpcNames::GetRandomName(CreateID(npc.faction.c_str()), npc.voiceId);
					pilotName.append_string(result.first);
					pilotName.append_string(result.second);
				}
			}
		}

		pub::Reputation::Alloc(shipInfo.iRep, formationName, pilotName);
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