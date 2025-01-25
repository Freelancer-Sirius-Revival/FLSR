#include "Main.h"
#include <random>

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
* nickname = foobar ;must be unique across all system solars. Can match with object nickname in NoCloakArea to enable de-cloak ability.
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
	static std::random_device rd;
	static std::mt19937 gen(rd());

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
			CSolar* solar = static_cast<CSolar*>(inspect->cobj);

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
				if (pPD->iShipID && pPD->iSystemID == solarInfo.iSystemID)
				{
					GetClientInterface()->Send_FLPACKET_SERVER_CREATESOLAR(pPD->iOnlineID, (FLPACKET_CREATESOLAR&)packetSolar);
					// Enforce an update to the client about their attitude to the spawned solar, or it will remain neutral after spawn.
					float attitudeValue;
					pub::Reputation::GetAttitude(solarInfo.iRep, pPD->iReputation, attitudeValue);
					pub::Reputation::SetAttitude(solarInfo.iRep, pPD->iReputation, attitudeValue);
				}
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
	static std::unordered_set<uint> spawnedSolars;

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
							const char* nickname;
							nickname = ini.get_value_string(0);
							if (strlen(nickname) > 0)
								solar.headId = CreateID(nickname);

							nickname = ini.get_value_string(1);
							if (strlen(nickname) > 0)
								solar.bodyId = CreateID(nickname);

							for (int index = 0; index < 8; index++) // The game supports up to 8 accessories
							{
								const char* accessoryNickname = ini.get_value_string(index + 2);
								if (strlen(accessoryNickname) == 0)
									break;
								solar.accessoryIds.push_back(CreateID(accessoryNickname));
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
		std::copy(info.accessoryIds.begin(), info.accessoryIds.end(), solarInfo.Costume.accessory);
		solarInfo.Costume.accessories = info.accessoryIds.size();
		solarInfo.iVoiceID = info.voiceId;
		solarInfo.baseId = info.baseId;
		std::string nicknameWithCounter;
		do {
			nicknameWithCounter = info.nickname + std::to_string(++info.nicknameCounter);
		} while (spawnedSolars.contains(CreateID(nicknameWithCounter.c_str())));
		strncpy_s(solarInfo.cNickName, sizeof(solarInfo.cNickName), nicknameWithCounter.c_str(), nicknameWithCounter.size());

		FmtStr infoname(info.idsName, 0);
		infoname.begin_mad_lib(info.idsName); // scanner name
		infoname.end_mad_lib();

		FmtStr infocard(info.idsInfocard, 0); // infocard is never displayed for dynamic solars
		infocard.begin_mad_lib(info.idsInfocard);
		infocard.end_mad_lib();

		pub::Reputation::Alloc(solarInfo.iRep, infoname, infocard);
		uint groupId;
		pub::Reputation::GetReputationGroup(groupId, info.affiliation.c_str());
		pub::Reputation::SetAffiliation(solarInfo.iRep, groupId);

		uint spaceObjId;
		SpawnSolar(spaceObjId, solarInfo);
		spawnedSolars.insert(spaceObjId);

		pub::AI::SetPersonalityParams personality;
		personality.state_graph = pub::StateGraph::get_state_graph("NOTHING", pub::StateGraph::TYPE_STANDARD);
		personality.state_id = true;
		pub::AI::SubmitState(spaceObjId, &personality);

		// This must use the general nickname to identify a no-cloak definition for this solar.
		Cloak::TryRegisterNoCloakSolar(info.nickname, spaceObjId, solarInfo.vPos, solarInfo.iSystemID);
	}

	static bool DestroySolar(const uint spaceObjId)
	{
		if (spawnedSolars.contains(spaceObjId) && pub::SpaceObj::ExistsAndAlive(spaceObjId) == 0) //0 means alive, -2 dead
		{
			pub::SpaceObj::Destroy(spaceObjId, DestroyType::VANISH);
			return true;
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

	struct LaunchComm
	{
		uint solarObjId;
		uint dockId;
	};
	static std::map<uint, LaunchComm> unprocessedLaunchComms;

	bool __stdcall Send_FLPACKET_SERVER_LAUNCH(uint iClientID, FLPACKET_LAUNCH& pLaunch)
	{
		returncode = DEFAULT_RETURNCODE;

		if (spawnedSolars.contains(pLaunch.iSolarObjId))
		{
			LaunchComm comm;
			comm.solarObjId = pLaunch.iSolarObjId;
			comm.dockId = pLaunch.iDock;
			unprocessedLaunchComms[iClientID] = comm;
		}
		return true;
	}

	static uint GetShipMessageId(uint shipId)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(shipId, inspect, starSystem))
			return 0;
		const Archetype::Ship* shipArchetype = static_cast<Archetype::Ship*>(inspect->cobj->archetype);
		char msgIdPrefix[64];
		strncpy_s(msgIdPrefix, sizeof(msgIdPrefix), shipArchetype->msgidprefix_str, shipArchetype->msgidprefix_len);
		return CreateID(msgIdPrefix);
	}

	static bool SendLaunchWellWishes(uint shipId, uint solarObjId, uint dockId)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(solarObjId, inspect, starSystem))
			return false;
		const CEqObj* solar = static_cast<CEqObj*>(inspect->cobj);
		const Archetype::EqObj* solarArchetype = static_cast<Archetype::EqObj*>(solar->archetype);
		Archetype::DockType dockType;
		try
		{
			dockType = solarArchetype->dockInfo.at(dockId).dockType;
		}
		catch(const std::out_of_range& e)
		{
			return false;
		}

		std::string clearMessageIdBase;
		switch (dockType)
		{
			case Archetype::DockType::Berth:
				clearMessageIdBase = "gcs_docklaunch_clear_berth_0";
				break;

			case Archetype::DockType::MoorSmall:
			case Archetype::DockType::MoorMedium:
			case Archetype::DockType::MoorLarge:
				clearMessageIdBase = "gcs_docklaunch_clear_moor_0";
				break;

			case Archetype::DockType::Ring:
				clearMessageIdBase = "gcs_docklaunch_clear_ring_0";
				break;

			default:
				return false;
		}

		static std::uniform_int_distribution<> distr(1, 2);
		std::vector<uint> lines = {
			GetShipMessageId(shipId),
			CreateID((clearMessageIdBase + std::to_string(distr(gen)) + "-").c_str()),
			CreateID(("gcs_misc_wellwish_0" + std::to_string(distr(gen)) + "-").c_str())
		};
		pub::SpaceObj::SendComm(solar->id, shipId, solar->voiceId, &solar->commCostume, 0, lines.data(), lines.size(), 19007 /* base comms type*/, 0.5f, false);
		return true;
	}

	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (!unprocessedLaunchComms.contains(clientId))
			return;

		SendLaunchWellWishes(shipId, unprocessedLaunchComms[clientId].solarObjId, unprocessedLaunchComms[clientId].dockId);
		unprocessedLaunchComms.erase(clientId);
	}

	static std::unordered_map<uint, std::unordered_set<uint>> dockQueues;

	// Gets called whenever a dock request begins, ends, is cancelled, or the ship is destroyed/despawned. Does not get called when the station gets destroyed.
	int __cdecl Dock_Call_After(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, DOCK_HOST_RESPONSE response)
	{
		returncode = DEFAULT_RETURNCODE;

		if (!spawnedSolars.contains(dockTargetId))
		{
			return 0;
		}

		// dockPortIndex -1 means docking was cancelled.
		if (dockPortIndex < 0 || response == DOCK_HOST_RESPONSE::DOCK)
		{
			dockQueues[dockTargetId].erase(ship);
			return 0;
		}

		if (response == DOCK_HOST_RESPONSE::DOCK_IN_USE)
			dockQueues[dockTargetId].insert(ship);

		IObjRW* inspect;
		StarSystem* starSystem;
		uint targetId = dockTargetId;
		if (!GetShipInspect(targetId, inspect, starSystem))
			return 0;
		const CEqObj* solar = static_cast<CEqObj*>(inspect->cobj);
		const Archetype::EqObj* solarArchetype = static_cast<Archetype::EqObj*>(solar->archetype);
		Archetype::DockType dockType;
		try
		{
			dockType = solarArchetype->dockInfo.at(dockPortIndex).dockType;
		}
		catch (const std::out_of_range& e)
		{
			return false;
		}
		
		std::vector<uint> lines;
		switch (response)
		{
			case DOCK_HOST_RESPONSE::PROCEED_DOCK:
			{
				std::string dockTypeMessageId;
				switch (dockType)
				{
					case Archetype::DockType::Berth:
						dockTypeMessageId = "gcs_dockrequest_todock";
						break;

					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockTypeMessageId = "gcs_dockrequest_tomoor";
						break;

					case Archetype::DockType::Ring:
						dockTypeMessageId = "gcs_dockrequest_toland";
						break;

					default:
						dockTypeMessageId = "";
						break;
				}

				std::string dockTargetMessageId;
				switch (dockType)
				{
					case Archetype::DockType::Berth:
						dockTargetMessageId = "gcs_dockrequest_todock_number";
						break;

					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockTargetMessageId = "gcs_dockrequest_tomoor_number";
						break;

					case Archetype::DockType::Ring:
						dockTargetMessageId = "gcs_dockrequest_toland-";
						break;

					default:
						dockTargetMessageId = "";
						break;
				}


				std::string dockNumberMessageId;
				switch (dockType)
				{
					case Archetype::DockType::Berth:
					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockNumberMessageId = "gcs_misc_number_" + std::to_string(dockPortIndex + 1) + "-";
						break;

					default:
						dockNumberMessageId = "";
						break;
				}

				if (dockQueues[dockTargetId].contains(ship))
				{
					lines = {
						GetShipMessageId(ship),
						CreateID("gcs_dockrequest_nowcleared_01+"),
						CreateID((!dockTypeMessageId.empty() ? (dockTypeMessageId + "-") : "").c_str()),
						CreateID("gcs_dockrequest_proceed_01+"),
						CreateID(dockTargetMessageId.c_str()),
						CreateID(dockNumberMessageId.c_str())
					};
				}
				else
				{
					lines = {
						CreateID(("gcs_misc_ack_0" + std::to_string(std::uniform_int_distribution(1, 3)(gen)) + "-").c_str()),
						CreateID("gcs_dockrequest_yourrequest+"),
						CreateID(dockTypeMessageId.c_str()),
						CreateID("gcs_dockrequest_granted_01-"),
						CreateID("gcs_dockrequest_proceed_01+"),
						CreateID(dockTargetMessageId.c_str()),
						CreateID(dockNumberMessageId.c_str())
					};
				}
				break;
			}



			case DOCK_HOST_RESPONSE::DOCK_IN_USE:
			{
				lines = {
					CreateID(("gcs_misc_ack_0" + std::to_string(std::uniform_int_distribution(1, 3)(gen)) + "-").c_str()),
					CreateID("gcs_dockrequest_standby_01-"),
					CreateID("gcs_dockrequest_delayedreason_01-"),
					CreateID("gcs_dockrequest_willbecleared_01-")
				};
				break;
			}



			case DOCK_HOST_RESPONSE::DOCK_DENIED:
			{
				std::string dockTypeMessageId;
				switch (dockType)
				{
					case Archetype::DockType::Berth:
						dockTypeMessageId = "gcs_dockrequest_todock";
						break;

					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockTypeMessageId = "gcs_dockrequest_tomoor";
						break;

					case Archetype::DockType::Ring:
						dockTypeMessageId = "gcs_dockrequest_toland";
						break;

					default:
						dockTypeMessageId = "";
					break;
				}

				lines = {
					CreateID(("gcs_misc_ack_0" + std::to_string(std::uniform_int_distribution(1, 3)(gen)) + "-").c_str()),
					CreateID("gcs_dockrequest_yourrequest+"),
					CreateID(dockTypeMessageId.c_str()),
					CreateID("gcs_dockrequest_denied_01-"),
					CreateID("gcs_dockrequest_nofit_01-")
				};
				break;
			}



			case DOCK_HOST_RESPONSE::ACCESS_DENIED:
				lines = { CreateID("gcs_dockrequest_denied_01-") };
				break;
		}

		uint shipId = ship;
		pub::SpaceObj::SendComm(solar->id, shipId, solar->voiceId, &solar->commCostume, 0, lines.data(), lines.size(), 19007 /* base comms type*/, 0.5f, false);
		return 0;
	}

	void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
	{
		returncode = DEFAULT_RETURNCODE;

		const uint objId = killedObject->cobj->get_id();
		spawnedSolars.erase(objId);
		dockQueues.erase(objId);
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
			return false;
		}
		return false;
	}
}
