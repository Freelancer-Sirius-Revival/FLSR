#include <regex>
#include "../Main.h"
#include "../Empathies.h"
#include "LootProps.h"
#include "Missions.h"
#include "Conditions/CndTrue.h"
#include "Conditions/CndDestroyed.h"
#include "Conditions/CndDistVec.h"
#include "Conditions/CndSpaceEnter.h"
#include "Conditions/CndSpaceExit.h"
#include "Conditions/CndBaseEnter.h"
#include "Conditions/CndTimer.h"
#include "Conditions/CndCount.h"
#include "Conditions/CndCloaked.h"
#include "Actions/ActDebugMsg.h"
#include "Actions/ActActTrigger.h"
#include "Actions/ActChangeState.h"
#include "Actions/ActAddLabel.h"
#include "Actions/ActRemoveLabel.h"
#include "Actions/ActLightFuse.h"
#include "Actions/ActSpawnSolar.h"
#include "Actions/ActSpawnShip.h"
#include "Actions/ActEndMission.h"
#include "Actions/ActDestroy.h"
#include "Actions/ActPlaySoundEffect.h"
#include "Actions/ActPlayMusic.h"
#include "Actions/ActEtherComm.h"
#include "Actions/ActSendComm.h"
#include "Actions/ActSetNNObj.h"
#include "Actions/ActAdjAcct.h"
#include "Actions/ActAdjRep.h"
#include "Actions/ActAddCargo.h"
#include "Actions/ActGiveObjList.h"
#include "Actions/ActSetVibe.h"
#include "Objectives/ObjGotoArch.h"
#include "MissionBoard.h"

namespace Missions
{	
	void RegisterMissionToJobBoard(Mission& mission)
	{
		if (mission.offer.type != pub::GF::MissionType::Unknown && !mission.offer.bases.empty())
		{
			MissionBoard::MissionOffer offer;
			offer.type = mission.offer.type;
			offer.system = mission.offer.system;
			offer.group = mission.offer.group;
			offer.text = mission.offer.text;
			offer.reward = mission.offer.reward;
			mission.offerId = MissionBoard::AddMissionOffer(offer, mission.offer.bases);
		}
	}

	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void LoadSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string missionDirectory = std::string(currentDirectory) + "\\flhook_plugins\\missions\\";
		if (!std::filesystem::is_directory(missionDirectory))
			return;

		// Read all mission files
		uint lastMissionId = 0;

		const std::regex filePattern(".+\\.ini", std::regex_constants::ECMAScript | std::regex_constants::icase);
		for (const auto& entry : std::filesystem::directory_iterator(missionDirectory))
		{
			const std::string fileName = wstos(entry.path().filename());
			INI_Reader ini;
			if (std::regex_match(fileName, filePattern) && ini.open((missionDirectory + fileName).c_str(), false))
			{
				while (ini.read_header())
				{
					if (ini.is_header("Mission"))
					{
						std::string name = "";
						bool initiallyActive = false;
						MissionOffer offer;

						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
								name = ToLower(ini.get_value_string(0));
							else if (ini.is_value("InitState"))
								initiallyActive = ToLower(ini.get_value_string(0)) == "active";
							else if (ini.is_value("offer_type"))
							{
								const auto value = ToLower(ini.get_value_string(0));
								if (value == "destroyships")
									offer.type = pub::GF::MissionType::DestroyShips;
								else if (value == "destroyinstallation")
									offer.type = pub::GF::MissionType::DestroyInstallation;
								else if (value == "assassinate")
									offer.type = pub::GF::MissionType::Assassinate;
								else if (value == "destroycontraband")
									offer.type = pub::GF::MissionType::DestroyContraband;
								else if (value == "captureprisoner")
									offer.type = pub::GF::MissionType::CapturePrisoner;
								else if (value == "retrievecontraband")
									offer.type = pub::GF::MissionType::RetrieveContraband;
								else
									offer.type = pub::GF::MissionType::Unknown;
							}
							else if (ini.is_value("offer_target_system"))
								offer.system = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("offer_faction"))
								pub::Reputation::GetReputationGroup(offer.group, ini.get_value_string(0));
							else if (ini.is_value("offer_string_id"))
								offer.text = ini.get_value_int(0);
							else if (ini.is_value("offer_reward"))
								offer.reward = ini.get_value_int(0);
							else if (ini.is_value("offer_bases"))
							{
								for (int index = 0, len = ini.get_num_parameters(); index < len; index++)
									offer.bases.push_back(CreateIdOrNull(ini.get_value_string(index)));
							}
						}
						// Never automatically start missions which are offered on the mission board.
						if (offer.type != pub::GF::MissionType::Unknown)
							initiallyActive = false;

						if (!name.empty())
						{
							lastMissionId++;
							missions.try_emplace(lastMissionId, name, lastMissionId, initiallyActive);
							missions.at(lastMissionId).offer = offer;
						}
					}

					if (missions.empty())
						continue;

					if (ini.is_header("Npc"))
					{
						Npc npc;
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
							{
								npc.name = ToLower(ini.get_value_string(0));
							}
							else if (ini.is_value("archetype"))
							{
								npc.archetypeId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("loadout"))
							{
								npc.loadoutId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("state_graph"))
							{
								npc.stateGraph = ToLower(ini.get_value_string(0));
							}
							else if (ini.is_value("faction"))
							{
								npc.faction = ini.get_value_string(0);
							}
							else if (ini.is_value("pilot"))
							{
								npc.pilotId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("voice"))
							{
								npc.voiceId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("space_costume"))
							{
								npc.costume.head = CreateIdOrNull(ini.get_value_string(0));
								npc.costume.body = CreateIdOrNull(ini.get_value_string(1));
								npc.costume.accessories = 0;
								for (int index = 0; index < 8; index++) // The game supports up to 8 accessories
								{
									const char* accessoryNickname = ini.get_value_string(index + 2);
									if (strlen(accessoryNickname) == 0)
										break;
									npc.costume.accessory[index] = CreateID(accessoryNickname);
									npc.costume.accessories++;
								}
							}
							else if (ini.is_value("level"))
							{
								npc.level = ini.get_value_int(0);
							}
						}
						if (npc.archetypeId && !npc.name.empty() && !npc.stateGraph.empty())
							missions.at(lastMissionId).npcs.push_back(npc);
					}

					if (ini.is_header("MsnNpc"))
					{
						MsnNpc npc;
						npc.position.x = 0;
						npc.position.y = 0;
						npc.position.z = 0;
						npc.orientation = EulerMatrix(npc.position);

						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
								npc.name = ToLower(ini.get_value_string(0));
							else if (ini.is_value("string_id"))
								npc.idsName = ini.get_value_int(0);
							else if (ini.is_value("system"))
								npc.systemId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("position"))
								npc.position = ini.get_vector();
							else if (ini.is_value("rotate"))
								npc.orientation = EulerMatrix(ini.get_vector());
							else if (ini.is_value("npc"))
								npc.npcId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("hitpoints"))
								npc.hitpoints = ini.get_value_int(0);
							else if (ini.is_value("pilot_job"))
								npc.pilotJobId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("arrival_obj"))
								npc.startingObjId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("label"))
								npc.labels.insert(CreateIdOrNull(ini.get_value_string(0)));
						}
						if (npc.npcId && !npc.name.empty() && npc.systemId)
							missions.at(lastMissionId).msnNpcs.push_back(npc);
					}

					if (ini.is_header("MsnSolar"))
					{
						MsnSolar solar;
						solar.position.x = 0;
						solar.position.y = 0;
						solar.position.z = 0;
						solar.orientation = EulerMatrix(solar.position);

						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
								solar.name = ToLower(ini.get_value_string(0));
							else if (ini.is_value("string_id"))
								solar.idsName = ini.get_value_int(0);
							else if (ini.is_value("system"))
								solar.systemId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("position"))
								solar.position = ini.get_vector();
							else if (ini.is_value("rotate"))
								solar.orientation = EulerMatrix(ini.get_vector());
							else if (ini.is_value("archetype"))
								solar.archetypeId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("loadout"))
								solar.loadoutId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("hitpoints"))
								solar.hitpoints = ini.get_value_int(0);
							else if (ini.is_value("base"))
								solar.baseId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("faction"))
								solar.faction = ini.get_value_string(0);
							else if (ini.is_value("pilot"))
								solar.pilotId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("voice"))
								solar.voiceId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("space_costume"))
							{
								solar.costume.headId = CreateIdOrNull(ini.get_value_string(0));
								solar.costume.bodyId = CreateIdOrNull(ini.get_value_string(1));
								for (int index = 0; index < 8; index++) // The game supports up to 8 accessories
								{
									const char* accessoryNickname = ini.get_value_string(index + 2);
									if (strlen(accessoryNickname) == 0)
										break;
									solar.costume.accessoryIds.push_back(CreateID(accessoryNickname));
								}
							}
							else if (ini.is_value("label"))
								solar.labels.insert(CreateIdOrNull(ini.get_value_string(0)));
						}

						if (solar.archetypeId && !solar.name.empty() && solar.systemId)
						{
							missions.at(lastMissionId).solars.push_back(solar);
							SolarSpawn::SolarArchetype solarArch;
							solarArch.archetypeId = solar.archetypeId;
							solarArch.loadoutId = solar.loadoutId;
							solarArch.nickname = missions.at(lastMissionId).name + ":" + solar.name;
							solarArch.idsName = solar.idsName;
							solarArch.position = solar.position;
							solarArch.orientation = solar.orientation;
							solarArch.systemId = solar.systemId;
							solarArch.baseId = solar.baseId;
							solarArch.affiliation = solar.faction;
							solarArch.pilotId = solar.pilotId;
							solarArch.hitpoints = solar.hitpoints;
							solarArch.voiceId = solar.voiceId;
							solarArch.headId = solar.costume.headId;
							solarArch.bodyId = solar.costume.bodyId;
							solarArch.accessoryIds = std::vector(solar.costume.accessoryIds);
							SolarSpawn::AppendSolarArchetype(solarArch);
						}
					}

					if (ini.is_header("ObjList"))
					{
						std::string nickname = "";
						ObjectivesArchetype objectives;
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
								nickname = ini.get_value_string(0);
							else if (ini.is_value("GotoShip"))
							{
								ObjGotoArchetypePtr arch(new ObjGotoArchetype());
								arch->type = pub::AI::GotoOpType::Ship;
								const auto val = ToLower(ini.get_value_string(0));
								if (val == "goto_no_cruise")
									arch->movement = GotoMovement::NoCruise;
								else
									arch->movement = GotoMovement::Cruise;
								arch->targetObjNameOrId = CreateIdOrNull(ini.get_value_string(1));
								arch->range = ini.get_value_float(2);
								arch->thrust = ini.get_value_float(3);
								arch->objNameToWaitFor = CreateIdOrNull(ini.get_value_string(4));
								arch->startWaitDistance = ini.get_value_float(5);
								arch->endWaitDistance = ini.get_value_float(6);
								objectives.objectives.push_back({ ObjectiveType::Goto, arch });
							}
							else if (ini.is_value("GotoVec"))
							{
								ObjGotoArchetypePtr arch(new ObjGotoArchetype());
								arch->type = pub::AI::GotoOpType::Vec;
								const auto val = ToLower(ini.get_value_string(0));
								if (val == "goto_no_cruise")
									arch->movement = GotoMovement::NoCruise;
								else
									arch->movement = GotoMovement::Cruise;
								arch->position.x = ini.get_value_float(1);
								arch->position.y = ini.get_value_float(2);
								arch->position.z = ini.get_value_float(3);
								arch->range = ini.get_value_float(4);
								arch->thrust = ini.get_value_float(5);
								arch->objNameToWaitFor = CreateIdOrNull(ini.get_value_string(6));
								arch->startWaitDistance = ini.get_value_float(7);
								arch->endWaitDistance = ini.get_value_float(8);
								objectives.objectives.push_back({ ObjectiveType::Goto, arch });
							}
							else if (ini.is_value("GotoSpline"))
							{
								ObjGotoArchetypePtr arch(new ObjGotoArchetype());
								arch->type = pub::AI::GotoOpType::Spline;
								const auto val = ToLower(ini.get_value_string(0));
								if (val == "goto_no_cruise")
									arch->movement = GotoMovement::NoCruise;
								else
									arch->movement = GotoMovement::Cruise;
								for (byte index = 0; index < 4; index++)
								{
									const byte offset = index * 3;
									arch->spline[index].x = ini.get_value_float(1 + offset);
									arch->spline[index].y = ini.get_value_float(2 + offset);
									arch->spline[index].z = ini.get_value_float(3 + offset);
								}
								arch->range = ini.get_value_float(13);
								arch->thrust = ini.get_value_float(14);
								arch->objNameToWaitFor = CreateIdOrNull(ini.get_value_string(15));
								arch->startWaitDistance = ini.get_value_float(16);
								arch->endWaitDistance = ini.get_value_float(17);
								objectives.objectives.push_back({ ObjectiveType::Goto, arch });
							}
						}
						if (!nickname.empty())
							missions.at(lastMissionId).objectives.insert({ CreateID(nickname.c_str()), objectives });
					}

					if (ini.is_header("Trigger"))
					{
						const uint id = missions.at(lastMissionId).triggers.size();
						const uint missionId = lastMissionId;
						std::string name = "";
						bool initiallyActive = false;
						Trigger::TriggerRepeatable repeatable = Trigger::TriggerRepeatable::Off;
						ConditionPtr condition = nullptr;
						std::vector<ActionPtr> actions;

						const ConditionParent conditionParent(missionId, id);

						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
							{
								name = ToLower(ini.get_value_string(0));
							}
							else if (ini.is_value("InitState"))
							{
								initiallyActive = ToLower(ini.get_value_string(0)) == "active";
							}
							else if (ini.is_value("repeatable"))
							{
								const std::string& value = ToLower(ini.get_value_string(0));
								if (value == "auto")
									repeatable = Trigger::TriggerRepeatable::Auto;
								else if (value == "manual")
									repeatable = Trigger::TriggerRepeatable::Manual;
								else
									repeatable = Trigger::TriggerRepeatable::Off;
							}
							else if (ini.is_value("Cnd_True"))
							{
								condition = ConditionPtr(new CndTrue(conditionParent));
							}
							else if (ini.is_value("Cnd_Destroyed"))
							{
								uint objNameOrLabel = 0;
								CndDestroyed::DestroyCondition reason = CndDestroyed::DestroyCondition::ALL;
								uint killerNameOrLabel = 0;
								int targetCount = -1;

								objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								targetCount = ini.get_value_int(1);
								const std::string val = ToLower(ini.get_value_string(2));
								if (val == "explode")
									reason = CndDestroyed::DestroyCondition::EXPLODE;
								else if (val == "silent")
									reason = CndDestroyed::DestroyCondition::SILENT;
								else
									reason = CndDestroyed::DestroyCondition::ALL;
								killerNameOrLabel = std::strlen(ini.get_value_string(6)) ? CreateIdOrNull(ini.get_value_string(3)) : 0;

								if (objNameOrLabel)
									condition = ConditionPtr(new CndDestroyed(conditionParent, objNameOrLabel, reason, killerNameOrLabel, targetCount));
							}
							else if (ini.is_value("Cnd_DistVec"))
							{
								uint objNameOrLabel = 0;
								CndDistVec::DistanceCondition reason = CndDistVec::DistanceCondition::Inside;
								Vector position;
								float distance = 0;
								uint systemId = 0;

								reason = ToLower(ini.get_value_string(0)) == "outside" ? CndDistVec::DistanceCondition::Outside : CndDistVec::DistanceCondition::Inside;
								objNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								position.x = ini.get_value_float(2);
								position.y = ini.get_value_float(3);
								position.z = ini.get_value_float(4);
								distance = ini.get_value_float(5);
								systemId = CreateIdOrNull(ini.get_value_string(6));

								if (objNameOrLabel && systemId && distance > 0)
									condition = ConditionPtr(new CndDistVec(conditionParent, objNameOrLabel, reason, position, distance, systemId));
							}
							else if (ini.is_value("Cnd_SpaceEnter"))
							{
								uint objNameOrLabel = 0;
								uint systemId = 0;

								objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								systemId = CreateIdOrNull(ini.get_value_string(1));

								if (objNameOrLabel && systemId)
									condition = ConditionPtr(new CndSpaceEnter(conditionParent, objNameOrLabel, systemId));
							}
							else if (ini.is_value("Cnd_SpaceExit"))
							{
								uint objNameOrLabel = 0;
								uint systemId = 0;

								objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								systemId = CreateIdOrNull(ini.get_value_string(1));

								if (objNameOrLabel && systemId)
									condition = ConditionPtr(new CndSpaceExit(conditionParent, objNameOrLabel, systemId));
							}
							else if (ini.is_value("Cnd_BaseEnter"))
							{
								uint objNameOrLabel = 0;
								uint baseId = 0;

								objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								baseId = CreateIdOrNull(ini.get_value_string(1));

								if (objNameOrLabel)
									condition = ConditionPtr(new CndBaseEnter(conditionParent, objNameOrLabel, baseId));
							}
							else if (ini.is_value("Cnd_Timer"))
							{
								float lowerTimeInS = 0.0f;
								float upperTimeInS = 0.0f;

								lowerTimeInS = ini.get_value_float(0);
								upperTimeInS = ini.get_value_float(1);

								if (lowerTimeInS >= 0.0f && upperTimeInS >= 0.0f)
									condition = ConditionPtr(new CndTimer(conditionParent, lowerTimeInS, upperTimeInS));
							}
							else if (ini.is_value("Cnd_Count"))
							{
								uint label = 0;
								uint targetCount = 0;
								CndCount::CountComparator comparator = CndCount::CountComparator::Equal;

								label = CreateIdOrNull(ini.get_value_string(0));
								targetCount = ini.get_value_int(1);
								if(ini.get_num_parameters() > 2)
								{
									const auto& value = ToLower(ini.get_value_string(2));
									if (value == "less")
										comparator = CndCount::CountComparator::Less;
									else if (value == "greater")
										comparator = CndCount::CountComparator::Greater;
									else
										comparator = CndCount::CountComparator::Equal;
								}

								if (label && targetCount >= 0)
									condition = ConditionPtr(new CndCount(conditionParent, label, targetCount, comparator));
							}
							else if (ini.is_value("Cnd_Cloaked"))
							{
								uint objNameOrLabel = 0;
								bool cloaked = false;

								objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								cloaked = ini.get_value_bool(1);

								if (objNameOrLabel)
									condition = ConditionPtr(new CndCloaked(conditionParent, objNameOrLabel, cloaked));
							}
							else if (ini.is_value("Act_DebugMsg"))
							{
								ActDebugMsgPtr action(new ActDebugMsg());
								action->message = ini.get_value_string(0);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_EndMission"))
							{
								ActEndMissionPtr action(new ActEndMission());
								actions.push_back(action);
							}
							else if (ini.is_value("Act_ChangeState"))
							{
								ActChangeStatePtr action(new ActChangeState());
								action->state = ToLower(ini.get_value_string(0)) == "succeed" ? ChangeState::Succeed : ChangeState::Fail;
								action->failureStringId = ini.get_value_int(1);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_ActTrig"))
							{
								ActActTriggerPtr action(new ActActTrigger());
								action->nameId = CreateIdOrNull(ini.get_value_string(0));
								if (ini.get_num_parameters() > 1)
									action->probability = ini.get_value_float(1);
								action->activate = true;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_DeactTrig"))
							{
								ActActTriggerPtr action(new ActActTrigger());
								action->nameId = CreateIdOrNull(ini.get_value_string(0));
								if (ini.get_num_parameters() > 1)
									action->probability = ini.get_value_float(1);
								action->activate = false;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AddLabel"))
							{
								ActAddLabelPtr action(new ActAddLabel());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->label = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_RemoveLabel"))
							{
								ActRemoveLabelPtr action(new ActRemoveLabel());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->label = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_LightFuse"))
							{
								ActLightFusePtr action(new ActLightFuse());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->fuse = CreateIdOrNull(ini.get_value_string(1));
								action->timeOffset = ini.get_value_float(2);
								action->lifetimeOverride = ini.get_value_float(3);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SpawnSolar"))
							{
								ActSpawnSolarPtr action(new ActSpawnSolar());
								action->solarName = ToLower(ini.get_value_string(0));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SpawnShip"))
							{
								ActSpawnShipPtr action(new ActSpawnShip());
								action->msnNpcName = ToLower(ini.get_value_string(0));
								const auto objectivesName = ToLower(ini.get_value_string(1));
								action->objectivesId = objectivesName == "no_ol" ? 0 : CreateID(objectivesName.c_str());
								if (ini.get_num_parameters() > 2)
								{
									action->position.x = ini.get_value_float(2);
									action->position.y = ini.get_value_float(3);
									action->position.z = ini.get_value_float(4);
								}
								if (ini.get_num_parameters() > 5)
								{
									Vector orientation;
									orientation.x = ini.get_value_float(5);
									orientation.y = ini.get_value_float(6);
									orientation.z = ini.get_value_float(7);
									action->orientation = EulerMatrix(orientation);
								}
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Destroy"))
							{
								ActDestroyPtr action(new ActDestroy());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->destroyType = ToLower(ini.get_value_string(1)) == "explode" ? DestroyType::EXPLODE : DestroyType::VANISH;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_PlaySoundEffect"))
							{
								ActPlaySoundEffectPtr action(new ActPlaySoundEffect());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->soundId = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_PlayMusic"))
							{
								ActPlayMusicPtr action(new ActPlayMusic());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->music.spaceMusic = std::string(ini.get_value_string(1)) != "none" ? CreateIdOrNull(ini.get_value_string(1)) : 0;
								action->music.dangerMusic = std::string(ini.get_value_string(2)) != "none" ? CreateIdOrNull(ini.get_value_string(2)) : 0;
								action->music.battleMusic = std::string(ini.get_value_string(3)) != "none" ? CreateIdOrNull(ini.get_value_string(3)) : 0;
								action->music.overrideMusic = std::string(ini.get_value_string(4)) != "none" ? CreateIdOrNull(ini.get_value_string(4)) : 0;
								action->music.crossFadeDurationInS = ini.get_value_float(5);
								if (ini.get_num_parameters() > 6)
									action->music.playOnce = ini.get_value_bool(6);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Ethercomm"))
							{
								ActEtherCommPtr action(new ActEtherComm());
								action->name = CreateIdOrNull(ini.get_value_string(0));
								action->receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								action->senderVoiceId = CreateIdOrNull(ini.get_value_string(2));
								int pos = 3;
								while (!ini.is_value_empty(pos))
								{
									const char* val = ini.get_value_string(pos);
									// Make sure we do not go beyond the following numeric value.
									char* end;
									strtol(val, &end, 10);
									if (end != val)
										break;
									action->lines.push_back(CreateIdOrNull(val));
									pos++;
								}
								if (!ini.is_value_empty(pos++))
									action->delay = ini.get_value_float(pos - 1);
								if (ini.get_num_parameters() > pos)
									action->global = ini.get_value_bool(pos++);
								action->senderIdsName = ini.get_value_int(pos++);
								action->costume.head = CreateIdOrNull(ini.get_value_string(pos++));
								action->costume.body = CreateIdOrNull(ini.get_value_string(pos++));
								int count = 0;
								while (count < 8)
								{
									const auto val = ini.get_value_string(pos++);
									if (strlen(val) == 0)
										break;
									action->costume.accessory[count++] = CreateIdOrNull(val);
								}
								action->costume.accessories = count;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SendComm"))
							{
								ActSendCommPtr action(new ActSendComm());
								action->name = CreateIdOrNull(ini.get_value_string(0));
								action->receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								action->senderObjName = CreateIdOrNull(ini.get_value_string(2));
								int pos = 3;
								while (!ini.is_value_empty(pos))
								{
									const char* val = ini.get_value_string(pos);
									// Make sure we do not go beyond the following numeric value.
									char* end;
									strtol(val, &end, 10);
									if (end != val)
										break;
									action->lines.push_back(CreateIdOrNull(val));
									pos++;
								}
								if (!ini.is_value_empty(pos++))
									action->delay = ini.get_value_float(pos - 1);
								if (ini.get_num_parameters() > pos)
									action->global = ini.get_value_bool(pos);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SetNNObj"))
							{
								ActSetNNObjPtr action(new ActSetNNObj());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->message = ini.get_value_int(1);
								action->systemId = CreateIdOrNull(ini.get_value_string(2));
								action->position.x = ini.get_value_float(3);
								action->position.y = ini.get_value_float(4);
								action->position.z = ini.get_value_float(5);
								if (ini.get_num_parameters() > 6)
									action->bestRoute = ini.get_value_bool(6);
								action->targetObjName = CreateIdOrNull(ini.get_value_string(7));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AdjAcct"))
							{
								ActAdjAcctPtr action(new ActAdjAcct());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->cash = ini.get_value_int(1);
								if (ini.get_num_parameters() > 2)
									action->splitBetweenPlayers = ini.get_value_bool(2);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AdjRep"))
							{
								ActAdjRepPtr action(new ActAdjRep());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								pub::Reputation::GetReputationGroup(action->groupId, ini.get_value_string(1));
								const auto& val = ToLower(ini.get_value_string(2));
								if (val == "objectdestruction")
									action->reason = Empathies::ReputationChangeReason::ObjectDestruction;
								else if (val == "missionsuccess")
										action->reason = Empathies::ReputationChangeReason::MissionSuccess;
								else if (val == "missionfailure")
									action->reason = Empathies::ReputationChangeReason::MissionFailure;
								else if (val == "missionabortion")
									action->reason = Empathies::ReputationChangeReason::MissionAbortion;
								else
									action->change = ini.get_value_float(2);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AddCargo"))
							{
								ActAddCargoPtr action(new ActAddCargo());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->itemId = CreateIdOrNull(ini.get_value_string(1));
								action->count = std::max(0, ini.get_value_int(2));
								if (ini.get_num_parameters() > 3)
									action->missionFlagged = ini.get_value_bool(3);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_GiveObjList"))
							{
								ActGiveObjListPtr action(new ActGiveObjList());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->objectivesId = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SetVibe"))
							{
								ActSetVibePtr action(new ActSetVibe());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->targetObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								action->reputation = ini.get_value_float(2);
								actions.push_back(action);
							}
						}

						if (!name.empty())
						{
							missions.at(missionId).triggers.emplace_back(name, id, missionId, initiallyActive, repeatable);
							Trigger& trigger = missions.at(missionId).triggers.at(id);
							if (condition == nullptr)
								ConPrint(L"Trigger '" + stows(trigger.name) + L"' has no condition! Falling back to Cnd_True\n");
							trigger.condition = condition != nullptr ? condition : ConditionPtr(new CndTrue({ missionId, id }));
							trigger.actions = actions;
						}
					}
				}
				ini.close();
			}
		}

		for (auto& missionEntry : missions)
			RegisterMissionToJobBoard(missionEntry.second);
	}
	
	void StartMissionByOfferId(const uint offerId, const std::vector<uint>& clientIds)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.offerId == offerId)
			{
				auto& mission = missionEntry.second;
				const uint labelId = CreateID("players");
				for (const auto clientId : clientIds)
					mission.AddLabelToObject(MissionObject(MissionObjectType::Client, clientId), labelId);
				mission.Reset();
				mission.Start();
				return;
			}
		}
	}

	bool IsPartOfOfferedJob(const uint clientId)
	{
		for (const auto& mission : missions)
		{
			if (mission.second.offerId != 0 && mission.second.clientIds.contains(clientId))
				return true;
		}
		return false;
	}

	void RemoveClientFromCurrentOfferedJob(const uint clientId)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.offerId != 0 && missionEntry.second.clientIds.contains(clientId))
			{
				missionEntry.second.RemoveClient(clientId);
				return;
			}
		}
	}

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		LoadSettings();

		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.initiallyActive)
				missionEntry.second.Start();
		}
	}

	static void ClearMissions()
	{
		auto it = missions.begin();
		while (it != missions.end())
		{
			MissionBoard::DeleteMissionOffer(it->second.offerId);
			it = missions.erase(it);
		}
	}

	void __stdcall Shutdown()
	{
		returncode = DEFAULT_RETURNCODE;
		ClearMissions();
	}

	static void RemoveObjectFromMissions(const uint objId)
	{
		// Removing an object from a mission could potentially end and remove it. So first gather the mission IDs and then delete them one by one.
		std::vector<uint> ids;
		for (const auto& entry : missions)
			ids.push_back(entry.first);

		for (const auto id : ids)
		{
			if (const auto& entry = missions.find(id); entry != missions.end())
				entry->second.RemoveObject(objId);
		}
	}

	static void RemoveClientFromMissions(const uint client)
	{
		// Removing a client from a mission could potentially end and remove it. So first gather the mission IDs and then delete them one by one.
		std::vector<uint> ids;
		for (const auto& entry : missions)
			ids.push_back(entry.first);

		for (const auto id : ids)
		{
			if (const auto& entry = missions.find(id); entry != missions.end())
				entry->second.RemoveClient(client);
		}
	}

	static void DestroyNonLootableEquipment(const IObjRW* killedObject, const uint killerId)
	{
		if (killedObject->is_player())
			return;

		for (const auto& mission : missions)
		{
			if (!mission.second.objectIds.contains(killedObject->cobj->id))
				continue;

			if (!(killedObject->cobj->objectClass & CObject::CEQOBJ_MASK))
				break;

			const auto eqObj = reinterpret_cast<CEqObj*>(killedObject->cobj);
			EquipDescVector equipList;
			eqObj->get_equip_desc_list(equipList);
			// Keep all lootable equip in cargo hold and only destroy the excess.
			if (HkGetClientIDByShip(killerId) > 0)
			{
				std::unordered_map<uint, int> countByLootableArchId;
				for (const auto& equipEntry : equipList.equip)
				{
					const auto& equipArch = Archetype::GetEquipment(equipEntry.iArchID);
					if (equipArch && equipArch->bLootable)
						countByLootableArchId[equipEntry.iArchID] += equipEntry.iCount;
				}

				for (auto& entry : countByLootableArchId)
					entry.second = LootProps::CalculateDropCount(entry.first, entry.second);

				for (const auto& equipEntry : equipList.equip)
				{
					if (equipEntry.bMission) // Always let mission equip drop, no matter what
						continue;

					const auto& equip = eqObj->equip_manager.FindByID(equipEntry.sID);
					if (!equip)
						continue;

					if (const auto& lootCount = countByLootableArchId.find(equipEntry.iArchID); lootCount != countByLootableArchId.end() && lootCount->second > 0)
					{
						if (equipEntry.iCount > static_cast<uint>(lootCount->second))
						{
							if (equip->CEquipType == EquipmentClass::Cargo)
								reinterpret_cast<CECargo*>(equip)->SetCount(lootCount->second);
							lootCount->second = 0;
						}
						else
						{
							lootCount->second -= equipEntry.iCount;
						}
					}
					else
					{
						equip->Destroy();
					}
				}
			}
			// Destroy all equipment in case an NPC was the killer.
			else
			{
				for (const auto& equipEntry : equipList.equip)
				{
					const auto& equip = eqObj->equip_manager.FindByID(equipEntry.sID);
					if (equip)
						equip->Destroy();
				}
			}
			break;
		}
	}

	static void ChangeReputationUponDestruction(const IObjRW* killedObject, const uint killerShipId)
	{
		if (killedObject->is_player())
			return;

		const uint killerClientId = HkGetClientIDByShip(killerShipId);
		if (!killerClientId)
			return;

		for (const auto& mission : missions)
		{
			if (!mission.second.objectIds.contains(killedObject->cobj->id))
				continue;

			uint victimReputationId = 0;
			if (killedObject->cobj->objectClass & CObject::CSHIP_OBJECT)
				victimReputationId = reinterpret_cast<CEqObj*>(killedObject->cobj)->repVibe;
			else if (killedObject->cobj->objectClass & CObject::CSOLAR_OBJECT)
				// Solars have their nickname ID directly mapped to their Reputation ID
				uint victimReputationId = killedObject->cobj->id;
			else
				return;

			uint victimGroupId;
			Reputation::Vibe::GetAffiliation(victimReputationId, victimGroupId, false);
			Empathies::ChangeReputationsByReason(killerClientId, victimGroupId, Empathies::ReputationChangeReason::ObjectDestruction);
			return;
		}
	}

	void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		returncode = DEFAULT_RETURNCODE;

		std::vector<CndDestroyed*> fulfilledDestructions;
		// Copy the original list because it might be modified implicitely by the following code
		for (const auto& cnd : destroyedConditions)
		{
			if (cnd->Matches(killedObject, killed, killerId))
				fulfilledDestructions.push_back(cnd);
		}

		std::vector<CndSpaceExit*> fulfilledSpaceExits;
		// For SpaceExit we do not care whether it happened by despawn (dock/leaving character) or death - both mean the same to NPCs and other Players.
		if (killedObject->is_player())
		{
			for (const auto& cnd : spaceExitConditions)
			{
				if (cnd->Matches(killedObject->cobj->ownerPlayer, killedObject->cobj->system))
					fulfilledSpaceExits.push_back(cnd);
			}
		}

		if (killed)
		{
			// Manually care for destruction of custom-spawned NPC equipment and cargo. Otherwise they loot everything always.
			DestroyNonLootableEquipment(killedObject, killerId);

			// Manually care for reputation change to the killer. Group-Rep will be handled anyway by the GroupRep module.
			ChangeReputationUponDestruction(killedObject, killerId);
		}

		RemoveObjectFromMissions(killedObject->cobj->id);

		// Execute those after the objects were unregistered from the mission. Otherwise e.g. respawning would be blocked because they "still exist".
		for (const auto& cnd : fulfilledDestructions)
		{
			if (const auto& foundCondition = destroyedConditions.contains(cnd))
				cnd->ExecuteTrigger();
		}
		for (const auto& cnd : fulfilledSpaceExits)
		{
			if (const auto& foundCondition = spaceExitConditions.contains(cnd))
				cnd->ExecuteTrigger();
		}
	}

	float elapsedTimeInSec = 0.0f;
	void __stdcall Elapse_Time_AFTER(float seconds)
	{
		returncode = DEFAULT_RETURNCODE;

		const std::unordered_set<CndTimer*> timerConditionsCopy(timerConditions);
		for (const auto& cnd : timerConditionsCopy)
		{
			if (const auto& foundCondition = timerConditions.find(cnd); foundCondition != timerConditions.end() && cnd->Matches(seconds))
				cnd->ExecuteTrigger();
		}

		elapsedTimeInSec += seconds;
		if (elapsedTimeInSec < 0.02f)
			return;
		elapsedTimeInSec = 0.0f;

		if (!cloakedConditions.empty())
		{
			const std::unordered_set<CndCloaked*> cloakedConditionsCopy(cloakedConditions);
			for (const auto cnd : cloakedConditionsCopy)
			{
				bool matchFound = false;
				struct PlayerData* playerData = 0;
				while (playerData = Players.traverse_active(playerData))
				{
					if (cnd->Matches(MissionObject(MissionObjectType::Client, playerData->iOnlineID)))
					{
						cnd->ExecuteTrigger();
						matchFound = true;
						break;
					}
				}
				if (!matchFound)
				{
					for (uint objId : missions.at(cnd->parent.missionId).objectIds)
					{
						if (cnd->Matches(MissionObject(MissionObjectType::Object, objId)))
						{
							cnd->ExecuteTrigger();
							break;
						}
					}
				}
			}
		}

		if (distVecConditions.empty())
			return;
		
		std::unordered_map<uint, DistVecMatchEntry> clientsByClientId;
		std::unordered_map<uint, DistVecMatchEntry> objectsByObjId;
		for (const auto cnd : distVecConditions)
		{
			const bool strangerRequested = cnd->objNameOrLabel == Stranger;
			if (strangerRequested || !missions.at(cnd->parent.missionId).clientIds.empty())
			{
				struct PlayerData* playerData = 0;
				while (playerData = Players.traverse_active(playerData))
				{
					if (clientsByClientId.contains(playerData->iOnlineID) || (!strangerRequested && !missions.at(cnd->parent.missionId).clientIds.contains(playerData->iOnlineID)))
						continue;

					uint shipId;
					pub::Player::GetShip(playerData->iOnlineID, shipId);
					if (shipId)
					{
						IObjRW* inspect;
						StarSystem* starSystem;
						if (!GetShipInspect(shipId, inspect, starSystem))
							continue;
						DistVecMatchEntry entry;
						entry.systemId = inspect->cobj->system;
						entry.position = inspect->cobj->vPos;
						clientsByClientId[playerData->iOnlineID] = entry;
					}
				}
			}
			if (!strangerRequested)
			{
				for (uint objId : missions.at(cnd->parent.missionId).objectIds)
				{
					if (objectsByObjId.contains(objId))
						continue;
					IObjRW* inspect;
					StarSystem* starSystem;
					if (!GetShipInspect(objId, inspect, starSystem))
						continue;
					DistVecMatchEntry entry;
					entry.systemId = inspect->cobj->system;
					entry.position = inspect->cobj->vPos;
					objectsByObjId[objId] = entry;
				}
			}
		}

		const std::unordered_set<CndDistVec*> distVecConditionsCopy(distVecConditions);
		for (const auto& cnd : distVecConditionsCopy)
		{
			if (const auto& foundCondition = distVecConditions.find(cnd); foundCondition != distVecConditions.end() && cnd->Matches(clientsByClientId, objectsByObjId))
				cnd->ExecuteTrigger();
		}
	}

	std::unordered_map<uint, CHARACTER_ID> lastCharacterByClientId;

	void __stdcall CharacterSelect(const CHARACTER_ID& cId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		const auto& foundEntry = lastCharacterByClientId.find(clientId);
		if (foundEntry == lastCharacterByClientId.end() || !(foundEntry->second == cId))
			RemoveClientFromMissions(clientId);
	}

	void __stdcall CharacterSelect_AFTER(const CHARACTER_ID& cId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		lastCharacterByClientId[clientId] = cId;
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2)
	{
		returncode = DEFAULT_RETURNCODE;
		lastCharacterByClientId.erase(clientId);
		RemoveClientFromMissions(clientId);
	}

	void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		if (spaceEnterConditions.empty())
			return;
		uint systemId;
		pub::Player::GetSystem(clientId, systemId);
		const std::unordered_set<CndSpaceEnter*> spaceEnterConditionsCopy(spaceEnterConditions);
		for (const auto& cnd : spaceEnterConditionsCopy)
		{
			if (const auto& foundCondition = spaceEnterConditions.find(cnd); foundCondition != spaceEnterConditions.end() && cnd->Matches(clientId, systemId))
				cnd->ExecuteTrigger();
		}
	}

	void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		const std::unordered_set<CndBaseEnter*> baseEnterConditionsCopy(baseEnterConditions);
		for (const auto& cnd : baseEnterConditionsCopy)
		{
			if (const auto& foundCondition = baseEnterConditions.find(cnd); foundCondition != baseEnterConditions.end() && cnd->Matches(clientId, baseId))
				cnd->ExecuteTrigger();
		}
	}

	static bool StartMission(const std::string& missionName)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.name == missionName)
				return missionEntry.second.Start();
		}
		return false;
	}

	static bool EndMission(const std::string& missionName)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.name == missionName)
			{
				missionEntry.second.End();
				return true;
			}
		}
		return false;
	}

	bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd)
	{
		returncode = DEFAULT_RETURNCODE;

		if (IS_CMD("start_mission"))
		{
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;

			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (StartMission(targetNickname))
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" started.");
			else
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" not found or already running.");
			return true;
		}
		else if (IS_CMD("stop_mission"))
		{
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;

			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (EndMission(targetNickname))
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" ended.");
			else
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" not found or already stopped.");
			return true;
		}
		else if (IS_CMD("reload_missions"))
		{
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;

			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			ClearMissions();
			initialized = false;
			PrintUserCmdText(clientId, L"Stopped and reloaded all missions.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL; // Must be set again because it gets unset somewhere before.
			return true;
		}
		return false;
	}
}