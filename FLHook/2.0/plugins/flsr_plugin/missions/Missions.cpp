#include <regex>
#include "../Main.h"
#include "NpcNames.h"
#include "../Empathies.h"
#include "LootProps.h"
#include "Missions.h"
#include "Mission.h"
#include "MissionArch.h"
#include "TriggerArch.h"
#include "Conditions/CndDestroyed.h"
#include "Conditions/CndDestroyedArch.h"
#include "Conditions/CndDistVec.h"
#include "Conditions/CndSpaceEnter.h"
#include "Conditions/CndSpaceExit.h"
#include "Conditions/CndBaseEnter.h"
#include "Conditions/CndTimer.h"
#include "Actions/ActActTrigger.h"
#include "Actions/ActAddLabel.h"
#include "Actions/ActRemoveLabel.h"
#include "Actions/ActLightFuseArch.h"
#include "Actions/ActSpawnSolarArch.h"
#include "Actions/ActSpawnShipArch.h"
#include "Actions/ActDestroyArch.h"
#include "Actions/ActPlaySoundEffectArch.h"
#include "Actions/ActPlayMusicArch.h"
#include "Actions/ActEtherCommArch.h"
#include "Actions/ActSendCommArch.h"
#include "Actions/ActSetNNObjArch.h"
#include "Actions/ActAdjAcctArch.h"
#include "Actions/ActAddCargoArch.h"
#include "Actions/ActGiveObjListArch.h"
#include "Objectives/ObjGotoArch.h"

namespace Missions
{
	/*
	struct NpcDefintion
	{
		std::string name = "";
	};

	struct DialogArchetype
	{
		std::string name = "";
	};

	struct MsnRandEncArchetype
	{
		std::string name = "";
	};

	struct NNObjectiveArchetype
	{
		std::string name = "";
	};

	struct ObjListArchetype
	{
		std::string name = "";
	};

	struct SolarArchetype
	{
		std::string name = "";
	};

	struct ShipArchetype
	{
		std::string name = "";
	};

	struct LootArchetype
	{
		std::string name = "";
		uint archetypeId = 0;
		uint displayNameId = 1;
		Vector position;
		Vector orientation;
		Vector velocity;
		uint amount = 1;
		float relativeHealth = 1.0f;
		bool canJettison = false;
	};

	struct FormationArchetype
	{
		std::string name = "";
		Vector position;
		Vector orientation;
		std::string formationName = "";
		std::vector<std::string> shipNames;
	};
	*/
	
	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	void LoadSettings()
	{
		NpcNames::ReadFiles();
		LootProps::ReadFiles();

		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string missionDirectory = std::string(currentDirectory) + "\\flhook_plugins\\missions\\";
		if (!std::filesystem::is_directory(missionDirectory))
			return;

		// Read all mission files
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
						MissionArchetypePtr mission(new MissionArchetype());
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
								mission->name = ToLower(ini.get_value_string(0));
							else if (ini.is_value("InitState"))
								mission->active = ToLower(ini.get_value_string(0)) == "active";
						}
						missionArchetypes.push_back(mission);
					}

					if (missionArchetypes.empty())
						continue;

					if (ini.is_header("Npc"))
					{
						NpcArchetypePtr npc(new NpcArchetype());
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
							{
								npc->name = ToLower(ini.get_value_string(0));
							}
							else if (ini.is_value("archetype"))
							{
								npc->archetypeId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("loadout"))
							{
								npc->loadoutId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("state_graph"))
							{
								npc->stateGraph = ToLower(ini.get_value_string(0));
							}
							else if (ini.is_value("faction"))
							{
								npc->faction = ini.get_value_string(0);
							}
							else if (ini.is_value("pilot"))
							{
								npc->pilotId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("voice"))
							{
								npc->voiceId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("space_costume"))
							{
								npc->costume.head = CreateIdOrNull(ini.get_value_string(0));
								npc->costume.body = CreateIdOrNull(ini.get_value_string(1));
								npc->costume.accessories = 0;
								for (int index = 0; index < 8; index++) // The game supports up to 8 accessories
								{
									const char* accessoryNickname = ini.get_value_string(index + 2);
									if (strlen(accessoryNickname) == 0)
										break;
									npc->costume.accessory[index] = CreateID(accessoryNickname);
									npc->costume.accessories++;
								}
							}
							else if (ini.is_value("level"))
							{
								npc->level = ini.get_value_int(0);
							}
						}
						if (npc->archetypeId && !npc->name.empty() && !npc->stateGraph.empty())
							missionArchetypes.back()->npcs.push_back(npc);
					}

					if (ini.is_header("MsnNpc"))
					{
						MsnNpcArchetypePtr npc(new MsnNpcArchetype());
						npc->position.x = 0;
						npc->position.y = 0;
						npc->position.z = 0;
						npc->orientation = EulerMatrix(npc->position);

						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
								npc->name = ToLower(ini.get_value_string(0));
							else if (ini.is_value("string_id"))
								npc->idsName = ini.get_value_int(0);
							else if (ini.is_value("system"))
								npc->systemId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("position"))
								npc->position = ini.get_vector();
							else if (ini.is_value("rotate"))
								npc->orientation = EulerMatrix(ini.get_vector());
							else if (ini.is_value("npc"))
								npc->npcId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("hitpoints"))
								npc->hitpoints = ini.get_value_int(0);
							else if (ini.is_value("pilot_job"))
								npc->pilotJobId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("arrival_obj"))
								npc->startingObjId = CreateIdOrNull(ini.get_value_string(0));
							else if (ini.is_value("label"))
								npc->labels.insert(CreateIdOrNull(ini.get_value_string(0)));
						}
						if (npc->npcId && !npc->name.empty() && npc->systemId)
							missionArchetypes.back()->msnNpcs.push_back(npc);
					}

					if (ini.is_header("MsnSolar"))
					{
						MsnSolarArchetypePtr solar(new MsnSolarArchetype());
						solar->position.x = 0;
						solar->position.y = 0;
						solar->position.z = 0;
						solar->orientation = EulerMatrix(solar->position);

						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
							{
								solar->name = ToLower(ini.get_value_string(0));
							}
							else if (ini.is_value("string_id"))
							{
								solar->idsName = ini.get_value_int(0);
							}
							else if (ini.is_value("system"))
							{
								solar->systemId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("position"))
							{
								solar->position = ini.get_vector();
							}
							else if (ini.is_value("rotate"))
							{
								solar->orientation = EulerMatrix(ini.get_vector());
							}
							else if (ini.is_value("archetype"))
							{
								solar->archetypeId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("loadout"))
							{
								solar->loadoutId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("base"))
							{
								solar->baseId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("faction"))
							{
								solar->faction = ini.get_value_string(0);
							}
							else if (ini.is_value("pilot"))
							{
								solar->pilotId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("voice"))
							{
								solar->voiceId = CreateIdOrNull(ini.get_value_string(0));
							}
							else if (ini.is_value("space_costume"))
							{
								solar->costume.headId = CreateIdOrNull(ini.get_value_string(0));
								solar->costume.bodyId = CreateIdOrNull(ini.get_value_string(1));
								for (int index = 0; index < 8; index++) // The game supports up to 8 accessories
								{
									const char* accessoryNickname = ini.get_value_string(index + 2);
									if (strlen(accessoryNickname) == 0)
										break;
									solar->costume.accessoryIds.push_back(CreateID(accessoryNickname));
								}
							}
							else if (ini.is_value("label"))
							{
								solar->labels.insert(CreateIdOrNull(ini.get_value_string(0)));
							}
						}

						if (solar->archetypeId && !solar->name.empty() && solar->systemId)
						{
							missionArchetypes.back()->solars.push_back(solar);
							SolarSpawn::SolarArchetype solarArch;
							solarArch.archetypeId = solar->archetypeId;
							solarArch.loadoutId = solar->loadoutId;
							solarArch.nickname = missionArchetypes.back()->name + ":" + solar->name;
							solarArch.idsName = solar->idsName;
							solarArch.position = solar->position;
							solarArch.orientation = solar->orientation;
							solarArch.systemId = solar->systemId;
							solarArch.baseId = solar->baseId;
							solarArch.affiliation = solar->faction;
							solarArch.pilotId = solar->pilotId;
							solarArch.hitpointsPercentage = solar->hitpointsPercentage;
							solarArch.voiceId = solar->voiceId;
							solarArch.headId = solar->costume.headId;
							solarArch.bodyId = solar->costume.bodyId;
							solarArch.accessoryIds = std::vector(solar->costume.accessoryIds);
							SolarSpawn::AppendSolarArchetype(solarArch);
						}
					}

					if (ini.is_header("ObjList"))
					{
						std::string nickname = "";
						ObjectivesArchetypePtr objectives(new ObjectivesArchetype());
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
								objectives->objectives.push_back({ ObjectiveType::Goto, arch });
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
								objectives->objectives.push_back({ ObjectiveType::Goto, arch });
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
								objectives->objectives.push_back({ ObjectiveType::Goto, arch });
							}
						}
						if (!nickname.empty())
							missionArchetypes.back()->objectives.insert({ CreateID(nickname.c_str()), objectives });
					}

					if (ini.is_header("Trigger"))
					{
						TriggerArchetypePtr trigger(new TriggerArchetype());
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
							{
								trigger->name = ToLower(ini.get_value_string(0));
							}
							else if (ini.is_value("InitState"))
							{
								trigger->active = ToLower(ini.get_value_string(0)) == "active";
							}
							else if (ini.is_value("repeatable"))
							{
								trigger->repeatable = ini.get_bool(0);
							}
							else if (ini.is_value("Cnd_True"))
							{
								trigger->condition = { TriggerCondition::Cnd_True, nullptr };
							}
							else if (ini.is_value("Cnd_Destroyed"))
							{
								CndDestroyedArchetypePtr archetype(new CndDestroyedArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->count = ini.get_value_int(1);
								const std::string val = ToLower(ini.get_value_string(2));
								if (val == "explode")
									archetype->condition = DestroyedCondition::EXPLODE;
								else if (val == "silent")
									archetype->condition = DestroyedCondition::SILENT;
								else
									archetype->condition = DestroyedCondition::ALL;
								archetype->killerNameOrLabel = std::strlen(ini.get_value_string(6)) ? CreateIdOrNull(ini.get_value_string(3)) : 0;
								trigger->condition = { TriggerCondition::Cnd_Destroyed, archetype };
							}
							else if (ini.is_value("Cnd_DistVec"))
							{
								CndDistVecArchetypePtr archetype(new CndDistVecArchetype());
								archetype->type = ToLower(ini.get_value_string(0)) == "outside" ? DistanceCondition::Outside : DistanceCondition::Inside;
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								archetype->position.x = ini.get_value_float(2);
								archetype->position.y = ini.get_value_float(3);
								archetype->position.z = ini.get_value_float(4);
								archetype->distance = ini.get_value_float(5);
								archetype->systemId = CreateIdOrNull(ini.get_value_string(6));
								trigger->condition = { TriggerCondition::Cnd_DistVec, archetype };
							}
							else if (ini.is_value("Cnd_SpaceEnter"))
							{
								CndSpaceEnterArchetypePtr archetype(new CndSpaceEnterArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->systemId = CreateIdOrNull(ini.get_value_string(1));
								trigger->condition = { TriggerCondition::Cnd_SpaceEnter, archetype };
							}
							else if (ini.is_value("Cnd_SpaceExit"))
							{
								CndSpaceExitArchetypePtr archetype(new CndSpaceExitArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->systemId = CreateIdOrNull(ini.get_value_string(1));
								trigger->condition = { TriggerCondition::Cnd_SpaceExit, archetype };
							}
							else if (ini.is_value("Cnd_BaseEnter"))
							{
								CndBaseEnterArchetypePtr archetype(new CndBaseEnterArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->baseId = CreateIdOrNull(ini.get_value_string(1));
								trigger->condition = { TriggerCondition::Cnd_BaseEnter, archetype };
							}
							else if (ini.is_value("Cnd_Timer"))
							{
								CndTimerArchetypePtr archetype(new CndTimerArchetype());
								archetype->timeInS = ini.get_value_float(0);
								trigger->condition = { TriggerCondition::Cnd_Timer, archetype };
							}
							else if (ini.is_value("Act_EndMission"))
							{
								trigger->actions.push_back({ TriggerAction::Act_EndMission, nullptr });
							}
							else if (ini.is_value("Act_ActTrig"))
							{
								ActActTriggerArchetypePtr archetype(new ActActTriggerArchetype());
								archetype->triggerName = ToLower(ini.get_value_string(0));
								archetype->activate = true;
								trigger->actions.push_back({ TriggerAction::Act_ActTrig, archetype });
							}
							else if (ini.is_value("Act_DeactTrig"))
							{
								ActActTriggerArchetypePtr archetype(new ActActTriggerArchetype());
								archetype->triggerName = ToLower(ini.get_value_string(0));
								archetype->activate = false;
								trigger->actions.push_back({ TriggerAction::Act_DeactTrig, archetype });
							}
							else if (ini.is_value("Act_AddLabel"))
							{
								ActAddLabelArchetypePtr archetype(new ActAddLabelArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->label = CreateIdOrNull(ini.get_value_string(1));
								trigger->actions.push_back({ TriggerAction::Act_AddLabel, archetype });
							}
							else if (ini.is_value("Act_RemoveLabel"))
							{
								ActRemoveLabelArchetypePtr archetype(new ActRemoveLabelArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->label = CreateIdOrNull(ini.get_value_string(1));
								trigger->actions.push_back({ TriggerAction::Act_RemoveLabel, archetype });
							}
							else if (ini.is_value("Act_LightFuse"))
							{
								ActLightFuseArchetypePtr archetype(new ActLightFuseArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->fuseName = ini.get_value_string(1);
								trigger->actions.push_back({ TriggerAction::Act_LightFuse, archetype });
							}
							else if (ini.is_value("Act_SpawnSolar"))
							{
								ActSpawnSolarArchetypePtr archetype(new ActSpawnSolarArchetype());
								archetype->solarName = ToLower(ini.get_value_string(0));
								trigger->actions.push_back({ TriggerAction::Act_SpawnSolar, archetype });
							}
							else if (ini.is_value("Act_SpawnShip"))
							{
								ActSpawnShipArchetypePtr archetype(new ActSpawnShipArchetype());
								archetype->msnNpcName = ToLower(ini.get_value_string(0));
								const auto objectivesName = ToLower(ini.get_value_string(1));
								archetype->objectivesId = objectivesName == "no_ol" ? 0 : CreateID(objectivesName.c_str());
								if (ini.get_num_parameters() > 2)
								{
									archetype->position.x = ini.get_value_float(2);
									archetype->position.y = ini.get_value_float(3);
									archetype->position.z = ini.get_value_float(4);
								}
								if (ini.get_num_parameters() > 5)
								{
									Vector orientation;
									orientation.x = ini.get_value_float(5);
									orientation.y = ini.get_value_float(6);
									orientation.z = ini.get_value_float(7);
									archetype->orientation = EulerMatrix(orientation);
								}
								trigger->actions.push_back({ TriggerAction::Act_SpawnShip, archetype });
								}
							else if (ini.is_value("Act_Destroy"))
							{
								ActDestroyArchetypePtr archetype(new ActDestroyArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->destroyType = ToLower(ini.get_value_string(1)) == "explode" ? DestroyType::EXPLODE : DestroyType::VANISH;
								trigger->actions.push_back({ TriggerAction::Act_Destroy, archetype });
							}
							else if (ini.is_value("Act_PlaySoundEffect"))
							{
								ActPlaySoundEffectArchetypePtr archetype(new ActPlaySoundEffectArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->soundId = CreateIdOrNull(ini.get_value_string(1));
								trigger->actions.push_back({ TriggerAction::Act_PlaySoundEffect, archetype });
							}
							else if (ini.is_value("Act_PlayMusic"))
							{
								ActPlayMusicArchetypePtr archetype(new ActPlayMusicArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->music.spaceMusic = std::string(ini.get_value_string(1)) != "none" ? CreateIdOrNull(ini.get_value_string(1)) : 0;
								archetype->music.dangerMusic = std::string(ini.get_value_string(2)) != "none" ? CreateIdOrNull(ini.get_value_string(2)) : 0;
								archetype->music.battleMusic = std::string(ini.get_value_string(3)) != "none" ? CreateIdOrNull(ini.get_value_string(3)) : 0;
								archetype->music.overrideMusic = std::string(ini.get_value_string(4)) != "none" ? CreateIdOrNull(ini.get_value_string(4)) : 0;
								archetype->music.crossFadeDurationInS = ini.get_value_float(5);
								archetype->music.playOnce = ini.get_bool(6);
								trigger->actions.push_back({ TriggerAction::Act_PlayMusic, archetype });
							}
							else if (ini.is_value("Act_Ethercomm"))
							{
								ActEtherCommArchetypePtr archetype(new ActEtherCommArchetype());
								archetype->name = CreateIdOrNull(ini.get_value_string(0));
								archetype->receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								archetype->senderVoiceId = CreateIdOrNull(ini.get_value_string(2));
								int pos = 3;
								while (!ini.is_value_empty(pos))
								{
									const char* val = ini.get_value_string(pos);
									// Make sure we do not go beyond the following numeric value.
									char* end;
									strtol(val, &end, 10);
									if (end != val)
										break;
									archetype->lines.push_back(CreateIdOrNull(val));
									pos++;
								}
								if (!ini.is_value_empty(pos++))
									archetype->delay = ini.get_value_float(pos - 1);
								archetype->global = ini.get_value_bool(pos++);
								archetype->senderIdsName = ini.get_value_int(pos++);
								archetype->costume.head = CreateIdOrNull(ini.get_value_string(pos++));
								archetype->costume.body = CreateIdOrNull(ini.get_value_string(pos++));
								int count = 0;
								while (count < 8)
								{
									const auto val = ini.get_value_string(pos++);
									if (strlen(val) == 0)
										break;
									archetype->costume.accessory[count++] = CreateIdOrNull(val);
								}
								archetype->costume.accessories = count;
								trigger->actions.push_back({ TriggerAction::Act_EtherComm, archetype });
							}
							else if (ini.is_value("Act_SendComm"))
							{
								ActSendCommArchetypePtr archetype(new ActSendCommArchetype());
								archetype->name = CreateIdOrNull(ini.get_value_string(0));
								archetype->receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								archetype->senderObjName = CreateIdOrNull(ini.get_value_string(2));
								int pos = 3;
								while (!ini.is_value_empty(pos))
								{
									const char* val = ini.get_value_string(pos);
									// Make sure we do not go beyond the following numeric value.
									char* end;
									strtol(val, &end, 10);
									if (end != val)
										break;
									archetype->lines.push_back(CreateIdOrNull(val));
									pos++;
								}
								if (!ini.is_value_empty(pos++))
									archetype->delay = ini.get_value_float(pos - 1);
								archetype->global = ini.get_value_bool(pos);
								trigger->actions.push_back({ TriggerAction::Act_SendComm, archetype });
							}
							else if (ini.is_value("Act_SetNNObj"))
							{
								ActSetNNObjArchetypePtr archetype(new ActSetNNObjArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->message = ini.get_value_int(1);
								archetype->systemId = CreateIdOrNull(ini.get_value_string(2));
								archetype->position.x = ini.get_value_float(3);
								archetype->position.y = ini.get_value_float(4);
								archetype->position.z = ini.get_value_float(5);
								archetype->bestRoute = ini.get_bool(6);
								archetype->targetObjName = CreateIdOrNull(ini.get_value_string(7));
								trigger->actions.push_back({ TriggerAction::Act_SetNNObj, archetype });
							}
							else if (ini.is_value("Act_AdjAcct"))
							{
								ActAdjAcctArchetypePtr archetype(new ActAdjAcctArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->cash = ini.get_value_int(1);
								trigger->actions.push_back({ TriggerAction::Act_AdjAcct, archetype });
							}
							else if (ini.is_value("Act_AddCargo"))
							{
								ActAddCargoArchetypePtr archetype(new ActAddCargoArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->itemId = CreateIdOrNull(ini.get_value_string(1));
								archetype->count = std::max(0, ini.get_value_int(2));
								archetype->missionFlagged = ini.get_value_bool(3);
								trigger->actions.push_back({ TriggerAction::Act_AddCargo, archetype });
							}
							else if (ini.is_value("Act_GiveObjList"))
							{
								ActGiveObjListArchetypePtr archetype(new ActGiveObjListArchetype());
								archetype->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								archetype->objectivesId = CreateIdOrNull(ini.get_value_string(1));
								trigger->actions.push_back({ TriggerAction::Act_GiveObjList, archetype });
							}
						}
						missionArchetypes.back()->triggers.push_back(trigger);
					}
				}
				ini.close();
			}
		}
	}

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		for (const auto& missionArchetype : missionArchetypes)
		{
			if (missionArchetype->active)
			{
				StartMission(missionArchetype->name);
			}
		}
	}

	static void DestroyNonLootingEquipment(const IObjRW* killedObject, const uint killerId)
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

			uint victimReputationId;
			if (killedObject->cobj->objectClass & CObject::CSHIP_OBJECT)
				victimReputationId = reinterpret_cast<CEqObj*>(killedObject->cobj)->repVibe;
			else if (killedObject->cobj->objectClass & CObject::CSOLAR_OBJECT)
				// Solars have their nickname ID directly mapped to their Reputation ID
				uint victimReputationId = killedObject->cobj->id;
			else
				return;

			uint victimGroupId;
			Reputation::Vibe::GetAffiliation(victimReputationId, victimGroupId, false);
			Empathies::ChangeReputationsByObjectDestruction(killerClientId, victimGroupId);
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
			DestroyNonLootingEquipment(killedObject, killerId);

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

		if (distVecConditions.empty())
			return;
		
		std::unordered_map<uint, DistVecMatchEntry> clientsByClientId;
		std::unordered_map<uint, DistVecMatchEntry> objectsByObjId;
		for (const auto cnd : distVecConditions)
		{
			const bool strangerRequested = cnd->archetype->objNameOrLabel == Stranger;
			if (strangerRequested || !missions[cnd->parent.missionId].clientIds.empty())
			{
				struct PlayerData* playerData = 0;
				while (playerData = Players.traverse_active(playerData))
				{
					if (clientsByClientId.contains(playerData->iOnlineID) || (!strangerRequested && !missions[cnd->parent.missionId].clientIds.contains(playerData->iOnlineID)))
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
				for (uint objId : missions[cnd->parent.missionId].objectIds)
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
		lastCharacterByClientId.insert({ clientId, cId });
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

	bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd)
	{
		returncode = DEFAULT_RETURNCODE;

		if (IS_CMD("start_mission"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (StartMission(targetNickname))
			{
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" started.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}
		}
		else if (IS_CMD("stop_mission"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (KillMission(targetNickname))
			{
				PrintUserCmdText(clientId, L"Ended mission " + stows(targetNickname));
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}
		}
		else if (IS_CMD("reload_missions"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			KillMissions();
			missionArchetypes.clear();
			LoadSettings();
			initialized = false;
			PrintUserCmdText(clientId, L"Ended and reloaded all missions");
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		return false;
	}
}