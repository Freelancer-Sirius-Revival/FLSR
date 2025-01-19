#include "Main.h"

/**
* Commands:
* Destroying a dynamically spawned solar: .kill (targetting via in-game selection)
* Spawning to pre-defined spot: .spawn <nickname>
* Spawning on current player location: .spawnhere <nickname>
* 
* Template:
* 
* [Solar]
* autospawn = false ;optional
* nickname = foobar
* archetype = largestation1
* loadout = cv_loadout_solar_largestation01; optional
* ids_name = 196663
* system = li01
* pos = -16419, 500, 75066
* rotate = 0, 170, 0
* reputation = rh_p_grp; optional
* space_costume =, robot_body_C; optional
* voice = atc_leg_f01a; optional
* pilot = pilot_solar_ace; optional
* base = li01_05_base; optional
*/

namespace SolarSpawn
{
	// This function is required to make sure the loadout is also sent to the clients.
	static void SpawnSolar(uint& spaceId, const pub::SpaceObj::SolarInfo& solarInfo)
	{
		// hack server.dll so it does not call create solar packet send
		char* serverHackAddress = (char*)hModServer + 0x2A62A;
		char serverHack[] = { '\xEB' };
		WriteProcMem(serverHackAddress, &serverHack, 1);

		pub::SpaceObj::CreateSolar(spaceId, solarInfo);

		StarSystem* starSystem;
		IObjRW* inspect;
		if (GetShipInspect(spaceId, inspect, starSystem))
		{
			CSolar* solar = (CSolar*)inspect->cobject();

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

			struct PlayerData* pPD = 0;
			while (pPD = Players.traverse_active(pPD))
			{
				if (pPD->iSystemID == solarInfo.iSystemID)
					GetClientInterface()->Send_FLPACKET_SERVER_CREATESOLAR(pPD->iOnlineID, (FLPACKET_CREATESOLAR&)packetSolar);
			}
		}

		// undo the server.dll hack
		char serverUnHack[] = { '\x74' };
		WriteProcMem(serverHackAddress, &serverUnHack, 1);
	}

	struct SolarArchetype
	{
		bool autospawn = false;
		uint archetypeId = 0;
		uint loadoutId = 0;
		std::string nickname;
		uint nicknameCounter = 0;
		uint idsName = 0;
		uint idsInfocard = 0;
		Vector position;
		Matrix orientation;
		uint systemId = 0;
		uint baseId = 0;
		std::string affiliation = "";
		uint personalityId = 0;
		float hitpointsPercentage = 1.0f;
		uint voiceId = 0;
		uint headId = 0;
		uint bodyId = 0;
		std::vector<uint> accessoryIds = {};
	};

	static std::vector<SolarArchetype> solarArchetypes;
	static std::unordered_map<std::string, uint> existingSolars;

	void LoadSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-Solars.cfg";

		INI_Reader ini;
		if (ini.open(configFilePath.c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Solar"))
				{
					SolarArchetype solar;
					solar.position.x = 0;
					solar.position.y = 0;
					solar.position.z = 0;
					solar.orientation = EulerMatrix(solar.position);

					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							solar.nickname = ini.get_value_string(0);
						else if (ini.is_value("autospawn"))
							solar.autospawn = ini.get_value_bool(0);
						else if (ini.is_value("archetype"))
							solar.archetypeId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("loadout"))
							solar.loadoutId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("ids_name"))
							solar.idsName = ini.get_value_int(0);
						else if (ini.is_value("system"))
							solar.systemId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("pos"))
							solar.position = ini.get_vector();
						else if (ini.is_value("rotate"))
							solar.orientation = EulerMatrix(ini.get_vector());
						else if (ini.is_value("base"))
							solar.baseId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("reputation"))
							solar.affiliation = ini.get_value_string(0);
						else if (ini.is_value("space_costume"))
						{
							solar.headId = CreateID(ini.get_value_string(0));
							solar.bodyId = CreateID(ini.get_value_string(1));
							for (int index = 0; index < 8; index++) // The game supports up to 8 accessories
							{
								const uint accessoryId = CreateID(ini.get_value_string(index + 2));
								if (accessoryId == 0)
									break;
								solar.accessoryIds.push_back(accessoryId);
							}
						}
						else if (ini.is_value("voice"))
							solar.voiceId = CreateID(ini.get_value_string(0));
						else if (ini.is_value("pilot"))
							solar.personalityId = CreateID(ini.get_value_string(0));
					}

					if (solar.archetypeId != 0 && !solar.nickname.empty())
						solarArchetypes.push_back(solar);
				}
			}
			ini.close();
		}
	}

	static void CreateSolar(SolarArchetype& info, const uint systemOverride = NULL, const Vector* positionOverride = NULL, const Matrix* orientationOverride = NULL)
	{
		pub::SpaceObj::SolarInfo solarInfo;
		memset(&solarInfo, 0, sizeof(solarInfo));
		solarInfo.iFlag = 4;
		solarInfo.iArchID = info.archetypeId;
		solarInfo.iLoadoutID = info.loadoutId;

		solarInfo.iHitPointsLeft = -1; // Max hit points from archetype are taken when -1
		solarInfo.iSystemID = systemOverride == NULL ? info.systemId : systemOverride;
		solarInfo.mOrientation = orientationOverride == NULL ? info.orientation : *orientationOverride;
		solarInfo.vPos = positionOverride == NULL ? info.position : *positionOverride;
		solarInfo.Costume.head = info.headId;
		solarInfo.Costume.body = info.bodyId;
		solarInfo.Costume.lefthand = 0;
		solarInfo.Costume.righthand = 0;
		std::copy(info.accessoryIds.begin(), info.accessoryIds.end(), solarInfo.Costume.accessory);
		solarInfo.Costume.accessories = info.accessoryIds.size();
		solarInfo.iVoiceID = info.voiceId;
		solarInfo.baseId = info.baseId;
		std::string nicknameWithCounter;
		do
		{
			nicknameWithCounter = info.nickname + std::to_string(++info.nicknameCounter);
		} while (existingSolars.contains(nicknameWithCounter));
		strncpy_s(solarInfo.cNickName, sizeof(solarInfo.cNickName), nicknameWithCounter.c_str(), nicknameWithCounter.size());

		// Set the base name
		FmtStr infoname(info.idsName, 0);
		infoname.begin_mad_lib(info.idsName); // scanner name
		infoname.end_mad_lib();

		FmtStr infocard(info.idsInfocard, 0);
		infocard.begin_mad_lib(info.idsInfocard); // infocard
		infocard.end_mad_lib();

		pub::Reputation::Alloc(solarInfo.iRep, infoname, infocard);
		uint groupId;
		pub::Reputation::GetReputationGroup(groupId, info.affiliation.c_str());
		pub::Reputation::SetAffiliation(solarInfo.iRep, groupId);

		uint spaceObjId;
		SpawnSolar(spaceObjId, solarInfo);
		existingSolars.insert({ nicknameWithCounter, spaceObjId });

		pub::AI::SetPersonalityParams personality;
		personality.state_graph = pub::StateGraph::get_state_graph("NOTHING", pub::StateGraph::TYPE_STANDARD);
		personality.state_id = true;
		pub::AI::get_personality(info.personalityId, personality.personality);
		pub::AI::SubmitState(spaceObjId, &personality);
	}

	static bool DestroySolar(const uint spaceObjId)
	{
		for (const auto& nicknameObjIdPair : existingSolars)
		{
			if (nicknameObjIdPair.second == spaceObjId)
			{
				if (pub::SpaceObj::ExistsAndAlive(spaceObjId) == 0) //0 means alive, -2 dead
					pub::SpaceObj::Destroy(spaceObjId, DestroyType::VANISH);
				return true;
			}
		}
		return false;
	}

	static bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		for (SolarArchetype& solar : solarArchetypes)
		{
			if (solar.autospawn)
				CreateSolar(solar);
		}
	}

	void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
	{
		returncode = DEFAULT_RETURNCODE;

		for (const auto& nicknameObjIdPair : existingSolars)
		{
			if (nicknameObjIdPair.second == killedObject->cobj->get_id())
			{
				existingSolars.erase(nicknameObjIdPair.first);
				break;
			}
		}
	}

	bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd)
	{
		returncode = DEFAULT_RETURNCODE;

		if (IS_CMD("kill"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			if (!shipId)
				return false;
			uint targetId;
			pub::SpaceObj::GetTarget(shipId, targetId);
			if (!targetId)
				return false;
			if (DestroySolar(targetId))
			{
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}
			return false;
		}

		if (IS_CMD("spawn"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			for (auto& solarArchetype : solarArchetypes)
			{
				if (ToLower(solarArchetype.nickname) == targetNickname)
				{
					CreateSolar(solarArchetype);
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return true;
				}
			}
			PrintUserCmdText(clientId, L"ERR solar not found: " + stows(targetNickname));
			return false;
		}

		if (IS_CMD("spawnhere"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			for (auto& solarArchetype : solarArchetypes)
			{
				if (ToLower(solarArchetype.nickname) == targetNickname)
				{
					uint shipId;
					pub::Player::GetShip(clientId, shipId);
					if (!shipId)
					{
						PrintUserCmdText(clientId, L"ERR must be in spoace");
						return false;
					}

					uint systemId;
					pub::Player::GetSystem(clientId, systemId);
					if (!systemId)
					{
						PrintUserCmdText(clientId, L"ERR must be a system");
						return false;
					}

					Vector shipVector;
					Matrix shipRotation;
					pub::SpaceObj::GetLocation(shipId, shipVector, shipRotation);

					CreateSolar(solarArchetype, systemId, &shipVector, &shipRotation);
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return true;
				}
			}
			PrintUserCmdText(clientId, L"ERR solar not found: " + stows(targetNickname));
		}
		return false;
	}
}
