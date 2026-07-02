#include "ActSpawnSolar.h"
#include "../MsnSolar.h"
#include "../SolarDocking.h"
#include "../../../../Pilots.h"
#include "../../../../NpcCloaking.h"
#include "../../../../Cloak.h"

namespace Missions
{
	// This can fail if there is already a solar at the very same coordinate in space.
	static uint TryCreateSolar(const pub::SpaceObj::SolarInfo& solarInfo)
	{
		__try
		{
			uint objId;
			pub::SpaceObj::CreateSolar(objId, solarInfo);
			return objId;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;
		}
	}

	// Freelancer comes with a bug where it forgets to send the loadout to the client. This function contains the fix to this. Only use this one for Solar Spawning.
	static uint CreateSolar(const pub::SpaceObj::SolarInfo& solarInfo)
	{
		// hack server.dll so it does not call create solar packet send
		char* serverHackAddress = (char*)hModServer + 0x2A62A;
		char serverHack[] = { '\xEB' };
		WriteProcMem(serverHackAddress, &serverHack, 1);

		const uint spaceId = TryCreateSolar(solarInfo);
		if (spaceId == 0)
			return 0;

		StarSystem* starSystem;
		IObjRW* inspect;
		if (GetShipInspect(spaceId, inspect, starSystem))
		{
			const CSolar* solar = static_cast<CSolar*>(inspect->cobj);

			// for every player in the same system, send solar creation packet
			struct SOLAR_STRUCT
			{
				byte starSystem[0x100];
			};

			SOLAR_STRUCT packetSolar;

			char* address1 = (char*)hModServer + 0x163F0;
			char* address2 = (char*)hModServer + 0x27950;

			// fill struct
			__asm
			{
				pushad
				lea ecx, packetSolar
				mov eax, address1
				call eax
				push solar
				lea ecx, packetSolar
				push ecx
				mov eax, address2
				call eax
				add esp, 8
				popad
			}

			PlayerData* player = nullptr;
			while (player = Players.traverse_active(player))
			{
				if (player->iShipID && player->iSystemID == solarInfo.iSystemID)
				{
					GetClientInterface()->Send_FLPACKET_SERVER_CREATESOLAR(player->iOnlineID, (FLPACKET_CREATESOLAR&)packetSolar);
					// Enforce an update to the client about their attitude to the spawned solar, or it will remain neutral after spawn.
					float attitudeValue;
					pub::Reputation::GetAttitude(solarInfo.iRep, player->iReputation, attitudeValue);
					pub::Reputation::SetAttitude(solarInfo.iRep, player->iReputation, attitudeValue);
				}
			}
		}

		// undo the server.dll hack
		char serverUnHack[] = { '\x74' };
		WriteProcMem(serverHackAddress, &serverUnHack, 1);

		return spaceId;
	}

	void ActSpawnSolar::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (mission.objectIdsByName.contains(solarId))
			return;

		const auto& entry = mission.msnSolars.find(solarId);
		if (entry == mission.msnSolars.end())
		{
			ConPrint(L"ERROR: MSN Solar " + std::to_wstring(solarId) + L" not found.\n");
			return;
		}
		const auto& solar = entry->second;
		if (solar.archetypeId == 0 || solar.name.empty())
			return;

		pub::SpaceObj::SolarInfo solarInfo;
		memset(&solarInfo, 0, sizeof(solarInfo));
		solarInfo.iFlag = 4;
		solarInfo.iArchID = solar.archetypeId;
		solarInfo.iLoadoutID = solar.loadoutId;
		solarInfo.iHitPointsLeft = solar.hitpoints;
		solarInfo.iSystemID = solar.systemId;
		solarInfo.mOrientation = orientation.data[0][0] != std::numeric_limits<float>::infinity() ? orientation : solar.orientation;
		solarInfo.vPos = position.x != std::numeric_limits<float>::infinity() ? position : solar.position;
		solarInfo.Costume.head = solar.costume.headId;
		solarInfo.Costume.body = solar.costume.bodyId;
		std::copy(solar.costume.accessoryIds.begin(), solar.costume.accessoryIds.end(), solarInfo.Costume.accessory);
		solarInfo.Costume.accessories = solar.costume.accessoryIds.size();
		solarInfo.iVoiceID = solar.voiceId;
		solarInfo.baseId = solar.baseId;
		strncpy_s(solarInfo.cNickName, (mission.name + ':' + solar.name).c_str(), sizeof(solarInfo.cNickName));

		pub::Reputation::Alloc(solarInfo.iRep, FmtStr(solar.idsName, 0), FmtStr(0, 0));
		uint groupId;
		pub::Reputation::GetReputationGroup(groupId, solar.faction.c_str());
		pub::Reputation::SetAffiliation(solarInfo.iRep, groupId);

		const uint objId = CreateSolar(solarInfo);
		if (objId == 0)
		{
			ConPrint(L"ERROR: Msn " + stows(mission.name) + L": Spawning solar " + stows(solarInfo.cNickName) + L" in " + std::to_wstring(solarInfo.iSystemID) + L" at " + std::to_wstring(solarInfo.vPos.x) + L", " + std::to_wstring(solarInfo.vPos.y) + L", " + std::to_wstring(solarInfo.vPos.z) + L" failed!");
			return;
		}

		pub::AI::SetPersonalityParams personality;
		personality.personality = Pilots::GetPilot(solar.pilotId);
		personality.state_graph = pub::StateGraph::get_state_graph("NOTHING", pub::StateGraph::TYPE_LEADER);
		personality.state_id = true;
		pub::AI::SubmitState(objId, &personality);

		const auto& solarArchetype = Archetype::GetSolar(solar.archetypeId);
		if (solarArchetype && !solarArchetype->bDestructible)
			// Invincibility kicks in at 99.999% hitpoints loss. This also prevents complete destruction of equipment.
			pub::SpaceObj::SetInvincible2(objId, true, true, 0.999f);

		mission.AddObject(objId, solar.id, solar.labels);
		RegisterDockableSolar(objId, solarInfo.baseId);
		Cloak::TryRegisterNoCloakSolar(mission.name + ':' + solar.name, objId);
		NpcCloaking::RegisterObject(objId);
	}
}