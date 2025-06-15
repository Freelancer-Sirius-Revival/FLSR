#include "ActSpawnShip.h"
#include "../../Pilots.h"
#include "../NpcNames.h"
#include "../Objectives/Objectives.h"

namespace Missions
{
	static uint TryCreateNpc(const pub::SpaceObj::ShipInfo& shipInfo)
	{
		__try
		{
			uint objId;
			pub::SpaceObj::Create(objId, shipInfo);
			return objId;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;
		}
	}

	static uint CreateNPC(const ActSpawnShip& action, const MsnNpcArchetype& msnNpc, const NpcArchetype& npc)
	{
		pub::SpaceObj::ShipInfo shipInfo;
		std::memset(&shipInfo, 0, sizeof(shipInfo));
		shipInfo.iFlag = 1;
		shipInfo.iSystem = msnNpc.systemId;
		shipInfo.iShipArchetype = npc.archetypeId;
		shipInfo.vPos = action.position.x != std::numeric_limits<float>::infinity() ? action.position : msnNpc.position;
		shipInfo.mOrientation = action.orientation.data[0][0] != std::numeric_limits<float>::infinity() ? action.orientation : msnNpc.orientation;
		shipInfo.iLoadout = npc.loadoutId;
		shipInfo.Costume = npc.costume;
		shipInfo.iPilotVoice = npc.voiceId;
		shipInfo.iHitPointsLeft = msnNpc.hitpoints;
		shipInfo.iLevel = npc.level;
		shipInfo.cargoDesc = new OwnerList<pub::SpaceObj::CargoDesc>();
		shipInfo.cargoDesc2 = new OwnerList<pub::SpaceObj::CargoDesc>();

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

		uint objId = TryCreateNpc(shipInfo);
		if (objId == 0)
		{
			ConPrint(L"Error spawning MSN NPC Ship " + stows(msnNpc.name) + L" in system " + std::to_wstring(msnNpc.systemId) + L" at position " + std::to_wstring(msnNpc.position.x) + L", " + std::to_wstring(msnNpc.position.y) + L", " + std::to_wstring(msnNpc.position.z) + L"\n");
			return 0;
		}

		pub::AI::SetPersonalityParams personality;
		personality.personality = msnNpc.pilotJobId ? Pilots::GetPilotWithJob(npc.pilotId, msnNpc.pilotJobId) : Pilots::GetPilot(npc.pilotId);
		personality.state_graph = pub::StateGraph::get_state_graph(npc.stateGraph.c_str(), pub::StateGraph::TYPE_STANDARD);
		personality.state_id = true;
		personality.contentCallback = 0;
		personality.directiveCallback = 0;
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

	void ActSpawnShip::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (mission.objectIdsByName.contains(CreateID(msnNpcName.c_str())))
			return;

		for (const auto& msnNpc : mission.archetype->msnNpcs)
		{
			if (msnNpc.name == msnNpcName)
			{
				for (const auto& npc : mission.archetype->npcs)
				{
					if (CreateID(npc.name.c_str()) == msnNpc.npcId)
					{
						const uint objId = CreateNPC(*this, msnNpc, npc);
						if (objId)
						{
							mission.AddObject(objId, CreateID(msnNpc.name.c_str()), msnNpc.labels);
							if (const auto& objectivesEntry = mission.archetype->objectives.find(objectivesId); objectivesEntry != mission.archetype->objectives.end())
							{
								const Objectives objectives(mission.id, objId, objectivesEntry->second.objectives);
								mission.objectivesByObjectId.insert({ objId, objectives });
								mission.objectivesByObjectId[objId].Progress();
							}
							return;
						}
						break;
					}
				}
				break;
			}
		}
	}
}