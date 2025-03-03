#include "../Main.h"
#include "Missions.h"
#include "Mission.h"
#include "MissionArch.h"
#include "TriggerArch.h"
#include "Conditions/CndDestroyed.h"
#include "Conditions/CndDestroyedArch.h"
#include "Conditions/CndDistVec.h"
#include "Actions/ActActTrigger.h"
#include "Actions/ActAddLabel.h"
#include "Actions/ActRemoveLabel.h"
#include "Actions/ActLightFuseArch.h"
#include "Actions/ActChangeStateArch.h"
#include "Actions/ActSpawnSolarArch.h"
#include "Actions/ActDestroyArch.h"

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
	void LoadSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-Missions.ini";

		INI_Reader ini;
		if (ini.open(configFilePath.c_str(), false))
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
							solar->systemId = CreateID(ini.get_value_string(0));
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
							solar->archetypeId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("loadout"))
						{
							solar->loadoutId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("base"))
						{
							solar->baseId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("faction"))
						{
							solar->faction = ini.get_value_string(0);
						}
						else if (ini.is_value("pilot"))
						{
							solar->pilotId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("voice"))
						{
							solar->voiceId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("space_costume"))
						{
							const char* nickname;
							nickname = ini.get_value_string(0);
							if (strlen(nickname) > 0)
								solar->costume.headId = CreateID(nickname);

							nickname = ini.get_value_string(1);
							if (strlen(nickname) > 0)
								solar->costume.bodyId = CreateID(nickname);

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
							solar->labels.insert(CreateID(ini.get_value_string(0)));
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
						solarArch.personalityId = solar->pilotId;
						solarArch.hitpointsPercentage = solar->hitpointsPercentage;
						solarArch.voiceId = solar->voiceId;
						solarArch.headId = solar->costume.headId;
						solarArch.bodyId = solar->costume.bodyId;
						solarArch.accessoryIds = std::vector(solar->costume.accessoryIds);
						SolarSpawn::AppendSolarArchetype(solarArch);
					}
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
							archetype->objNameOrLabel = CreateID(ini.get_value_string(0));
							archetype->count = ini.get_value_int(1);
							const std::string val = ToLower(ini.get_value_string(2));
							if (val == "explode")
								archetype->condition = DestroyedCondition::EXPLODE;
							else if (val == "silent")
								archetype->condition = DestroyedCondition::SILENT;
							else
								archetype->condition = DestroyedCondition::ALL;
							archetype->killerNameOrLabel = CreateID(ini.get_value_string(3));
							trigger->condition = { TriggerCondition::Cnd_Destroyed, archetype };
						}
						else if (ini.is_value("Cnd_DistVec"))
						{
							CndDistVecArchetypePtr archetype(new CndDistVecArchetype());
							archetype->type = ToLower(ini.get_value_string(0)) == "outside" ? DistanceCondition::OUTSIDE : DistanceCondition::INSIDE;
							archetype->objNameOrLabel = CreateID(ini.get_value_string(1));
							archetype->position.x = ini.get_value_float(2);
							archetype->position.y = ini.get_value_float(3);
							archetype->position.z = ini.get_value_float(4);
							archetype->distance = ini.get_value_float(5);
							archetype->systemId = CreateID(ini.get_value_string(6));
							trigger->condition = { TriggerCondition::Cnd_DistVec, archetype };
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
							archetype->objNameOrLabel = CreateID(ini.get_value_string(0));
							archetype->label = CreateID(ini.get_value_string(1));
							trigger->actions.push_back({ TriggerAction::Act_AddLabel, archetype });
						}
						else if (ini.is_value("Act_RemoveLabel"))
						{
							ActRemoveLabelArchetypePtr archetype(new ActRemoveLabelArchetype());
							archetype->objNameOrLabel = CreateID(ini.get_value_string(0));
							archetype->label = CreateID(ini.get_value_string(1));
							trigger->actions.push_back({ TriggerAction::Act_RemoveLabel, archetype });
						}
						else if (ini.is_value("Act_LightFuse"))
						{
							ActLightFuseArchetypePtr archetype(new ActLightFuseArchetype());
							archetype->objNameOrLabel = CreateID(ini.get_value_string(0));
							archetype->fuseId = CreateID(ini.get_value_string(1));
							trigger->actions.push_back({ TriggerAction::Act_LightFuse, archetype });
						}
						else if (ini.is_value("Act_ChangeState"))
						{
							ActChangeStateArchetypePtr archetype(new ActChangeStateArchetype());
							archetype->state = ToLower(ini.get_value_string(0)) == "succeed" ? MissionState::SUCCEED : MissionState::FAIL;
							archetype->failTextId = ini.get_value_int(1);
							trigger->actions.push_back({ TriggerAction::Act_ChangeState, archetype });
						}
						else if (ini.is_value("Act_SpawnSolar"))
						{
							ActSpawnSolarArchetypePtr archetype(new ActSpawnSolarArchetype());
							archetype->solarName = ToLower(ini.get_value_string(0));
							trigger->actions.push_back({ TriggerAction::Act_SpawnSolar, archetype });
						}
						else if (ini.is_value("Act_Destroy"))
						{
							ActDestroyArchetypePtr archetype(new ActDestroyArchetype());
							archetype->objNameOrLabel = CreateID(ini.get_value_string(0));
							archetype->destroyType = ToLower(ini.get_value_string(1)) == "explode" ? DestroyType::EXPLODE : DestroyType::VANISH;
							trigger->actions.push_back({ TriggerAction::Act_Destroy, archetype });
						}
					}
					missionArchetypes.back()->triggers.push_back(trigger);
				}
			}
			ini.close();
		}
	}

	static bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		for (const auto& missionArchetype: missionArchetypes)
		{
			if (missionArchetype->active)
			{
				StartMission(missionArchetype->name);
			}
		}
	}

	void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		returncode = DEFAULT_RETURNCODE;

		// Copy the original list because it might be modified implicitely by the following code
		std::unordered_set<CndDestroyed*> originals(destroyedConditions);
		for (const auto cnd : originals)
		{
			const auto foundCondition = destroyedConditions.find(cnd);
			if (foundCondition != destroyedConditions.end() && cnd->Matches(killedObject, killed, killerId))
			{
				cnd->trigger->QueueExecution();
			}
		}

		RemoveObjectFromMissions(killedObject->cobj->id);
	}

	struct FoundObject
	{
		uint systemId;
		Vector position;
	};

	float elapsedTimeInSec = 0.0f;
	void __stdcall Elapse_Time_AFTER(float seconds)
	{
		returncode = DEFAULT_RETURNCODE;

		elapsedTimeInSec += seconds;
		if (elapsedTimeInSec < 0.02f)
			return;
		elapsedTimeInSec = 0.0f;

		if (distVecConditions.empty())
			return;

		std::unordered_map<uint, FoundObject> objPositions;
		std::unordered_map<uint, FoundObject> clientPositions;
		std::unordered_map<CndDistVec*, std::vector<uint>> objIdsPerDistVec;
		std::unordered_map<CndDistVec*, std::vector<uint>> clientIdsPerDistVec;

		for (const auto cnd : distVecConditions)
		{
			if (cnd->archetype->objNameOrLabel == CreateID("stranger"))
			{
				struct PlayerData* playerData = 0;
				while (playerData = Players.traverse_active(playerData))
				{
					const uint clientId = playerData->iOnlineID;
					clientIdsPerDistVec[cnd].push_back(clientId);
					if (clientPositions.contains(clientId))
						continue;

					uint shipId;
					pub::Player::GetShip(clientId, shipId);
					if (shipId)
					{
						IObjRW* inspect;
						StarSystem* starSystem;
						if (!GetShipInspect(shipId, inspect, starSystem))
							break;
						clientPositions[clientId].systemId = inspect->cobj->system;
						clientPositions[clientId].position = inspect->cobj->vPos;
					}
				}
			}
			else
			{
				for (auto& object : cnd->trigger->mission->objects)
				{
					if (object.name == cnd->archetype->objNameOrLabel || object.labels.contains(cnd->archetype->objNameOrLabel))
					{
						if (object.clientId)
						{
							clientIdsPerDistVec[cnd].push_back(object.clientId);
							if (!clientPositions.contains(object.clientId))
							{
								uint shipId;
								pub::Player::GetShip(object.clientId, shipId);
								if (shipId)
								{
									IObjRW* inspect;
									StarSystem* starSystem;
									if (!GetShipInspect(shipId, inspect, starSystem))
										break;
									clientPositions[object.clientId].systemId = inspect->cobj->system;
									clientPositions[object.clientId].position = inspect->cobj->vPos;
								}
							}
						}
						else
						{
							objIdsPerDistVec[cnd].push_back(object.objId);
							if (!objPositions.contains(object.objId))
							{
								IObjRW* inspect;
								StarSystem* starSystem;
								if (!GetShipInspect(object.objId, inspect, starSystem))
									break;
								objPositions[object.objId].systemId = inspect->cobj->system;
								objPositions[object.objId].position = inspect->cobj->vPos;
							}
						}
					}
				}
			}
		}

		std::unordered_set<CndDistVec*> originals(distVecConditions);
		for (const auto cnd : originals)
		{
			std::vector<DistVecMatchEntry> entries;
			if (cnd->archetype->objNameOrLabel == CreateID("stranger"))
			{
				std::unordered_set<uint> presentClientIds;
				for (const auto& object : cnd->trigger->mission->objects)
					presentClientIds.insert(object.clientId);

				for (const auto clientId : clientIdsPerDistVec[cnd])
				{
					if (presentClientIds.contains(clientId))
						continue;

					const auto foundPosition = clientPositions.find(clientId);
					if (foundPosition != clientPositions.end() && foundPosition->second.systemId == cnd->archetype->systemId)
					{
						DistVecMatchEntry entry;
						entry.objId = 0;
						entry.clientId = clientId;
						entry.position = foundPosition->second.position;
						entries.push_back(entry);
					}
				}
			}
			else
			{
				for (const auto clientId : clientIdsPerDistVec[cnd])
				{
					const auto foundPosition = clientPositions.find(clientId);
					if (foundPosition != clientPositions.end() && foundPosition->second.systemId == cnd->archetype->systemId)
					{
						DistVecMatchEntry entry;
						entry.objId = 0;
						entry.clientId = clientId;
						entry.position = foundPosition->second.position;
						entries.push_back(entry);
					}
				}
				for (const auto objId : objIdsPerDistVec[cnd])
				{
					const auto foundPosition = objPositions.find(objId);
					if (foundPosition != objPositions.end() && foundPosition->second.systemId == cnd->archetype->systemId)
					{
						DistVecMatchEntry entry;
						entry.objId = objId;
						entry.clientId = 0;
						entry.position = foundPosition->second.position;
						entries.push_back(entry);
					}
				}
			}
			if (cnd->Matches(entries))
				cnd->trigger->QueueExecution();
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
		return false;
	}
}