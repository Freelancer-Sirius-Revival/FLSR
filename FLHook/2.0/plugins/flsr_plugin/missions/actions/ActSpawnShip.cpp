#include <FLHook.h>
#include "../../Pilots.h"
#include "../NpcNames.h"
#include "ActSpawnShip.h"
#include "../Objectives/Objectives.h"

namespace Missions
{
	ActSpawnShip::ActSpawnShip(const ActionParent& parent, const ActSpawnShipArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_SpawnShip),
		archetype(actionArchetype)
	{}

	static uint CreateNPC(const ActSpawnShipArchetype& actionArchetype, const MsnNpcArchetype& msnNpc, const NpcArchetype& npc)
	{
		pub::SpaceObj::ShipInfo shipInfo;
		memset(&shipInfo, 0, sizeof(shipInfo));
		shipInfo.iFlag = 1;
		shipInfo.iSystem = msnNpc.systemId;
		shipInfo.iShipArchetype = npc.archetypeId;
		shipInfo.vPos = actionArchetype.position.x != std::numeric_limits<float>::infinity() ? actionArchetype.position : msnNpc.position;
		shipInfo.mOrientation = actionArchetype.orientation.data[0][0] != std::numeric_limits<float>::infinity() ? actionArchetype.orientation : msnNpc.orientation;
		shipInfo.iLoadout = npc.loadoutId;
		shipInfo.Costume = npc.costume;
		shipInfo.iPilotVoice = npc.voiceId;
		shipInfo.iHitPointsLeft = msnNpc.hitpoints;
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
		return objId;
	}

	void ActSpawnShip::Execute()
	{
		auto& mission = missions[parent.missionId];
		ConPrint(stows(mission.archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_SpawnShip " + stows(archetype->msnNpcName));
		if (mission.objectIdsByName.contains(CreateID(archetype->msnNpcName.c_str())))
		{
			ConPrint(L" ALREADY EXISTS\n");
			return;
		}
		for (const auto& msnNpc : mission.archetype->msnNpcs)
		{
			if (msnNpc->name == archetype->msnNpcName)
			{
				for (const auto& npc : mission.archetype->npcs)
				{
					if (CreateID(npc->name.c_str()) == msnNpc->npcId)
					{
						const uint objId = CreateNPC(*archetype, *msnNpc, *npc);
						if (objId)
						{
							mission.objectIds.insert(objId);
							mission.objectIdsByName[CreateID(msnNpc->name.c_str())] = objId;
							for (const auto& label : msnNpc->labels)
							{
								MissionObject object;
								object.type = MissionObjectType::Object;
								object.id = objId;
								mission.objectsByLabel[label].push_back(object);
							}
							if (const auto& objectivesEntry = mission.archetype->objectives.find(archetype->objectivesId); objectivesEntry != mission.archetype->objectives.end())
							{
								const Objectives objectives(parent.missionId, objId, objectivesEntry->second->objectives);
								mission.objectivesByObjectId.insert({ objId, objectives });
								mission.objectivesByObjectId[objId].Progress();
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