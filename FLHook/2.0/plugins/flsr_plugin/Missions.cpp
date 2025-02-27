#include "Missions.h"
#include "Mission.h"
#include "MissionArch.h"
#include "TriggerArch.h"
#include "CndDestroyed.h"
#include "CndDestroyedArch.h"
#include "ActActTrigger.h"
#include "ActLightFuseArch.h"
#include "ActChangeStateArch.h"
#include "ActSpawnSolarArch.h"
#include "ActDestroyArch.h"

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
			MissionArchetype mission;
			while (ini.read_header())
			{
				if (ini.is_header("Mission"))
				{
					if (!mission.name.empty())
					{
						missionArchetypesByName[mission.name] = mission;
						mission = {};
					}

					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							mission.name = ini.get_value_string(0);
						else if (ini.is_value("InitState"))
							mission.active = ToLower(std::string(ini.get_value_string(0))) == "active";
						else if (ini.is_value("title"))
							mission.titleId = ini.get_value_int(0);
						else if (ini.is_value("offer"))
							mission.offerId = ini.get_value_int(0);
						else if (ini.is_value("reward"))
							mission.reward = ini.get_value_int(0);
					}
				}
				else if (mission.name.empty())
				{
					continue;
				}

				if (ini.is_header("MsnSolar"))
				{
					MsnSolarArchetype solar;
					solar.position.x = 0;
					solar.position.y = 0;
					solar.position.z = 0;
					solar.orientation = EulerMatrix(solar.position);

					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
						{
							solar.name = mission.name + ":" + std::string(ini.get_value_string(0));
						}
						else if (ini.is_value("string_id"))
						{
							solar.idsName = ini.get_value_int(0);
						}
						else if (ini.is_value("system"))
						{
							solar.systemId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("position"))
						{
							solar.position = ini.get_vector();
						}
						else if (ini.is_value("rotate"))
						{
							solar.orientation = EulerMatrix(ini.get_vector());
						}
						else if (ini.is_value("archetype"))
						{
							solar.archetypeId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("loadout"))
						{
							solar.loadoutId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("base"))
						{
							solar.baseId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("faction"))
						{
							solar.faction = ini.get_value_string(0);
						}
						else if (ini.is_value("pilot"))
						{
							solar.pilotId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("voice"))
						{
							solar.voiceId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("space_costume"))
						{
							const char* nickname;
							nickname = ini.get_value_string(0);
							if (strlen(nickname) > 0)
								solar.costume.headId = CreateID(nickname);

							nickname = ini.get_value_string(1);
							if (strlen(nickname) > 0)
								solar.costume.bodyId = CreateID(nickname);

							for (int index = 0; index < 8; index++) // The game supports up to 8 accessories
							{
								const char* accessoryNickname = ini.get_value_string(index + 2);
								if (strlen(accessoryNickname) == 0)
									break;
								solar.costume.accessoryIds.push_back(CreateID(accessoryNickname));
							}
						}
						else if (ini.is_value("label"))
						{
							solar.labels.insert(ini.get_value_string(0));
						}
					}

					if (solar.archetypeId && !solar.name.empty() && solar.systemId)
					{
						mission.solars.push_back(solar);
						SolarSpawn::SolarArchetype solarArch;
						solarArch.archetypeId = solar.archetypeId;
						solarArch.loadoutId = solar.loadoutId;
						solarArch.nickname = solar.name;
						solarArch.idsName = solar.idsName;
						solarArch.position = solar.position;
						solarArch.orientation = solar.orientation;
						solarArch.systemId = solar.systemId;
						solarArch.baseId = solar.baseId;
						solarArch.affiliation = solar.faction;
						solarArch.personalityId = solar.pilotId;
						solarArch.hitpointsPercentage = solar.hitpointsPercentage;
						solarArch.voiceId = solar.voiceId;
						solarArch.headId = solar.costume.headId;
						solarArch.bodyId = solar.costume.bodyId;
						solarArch.accessoryIds = std::vector(solar.costume.accessoryIds);
						SolarSpawn::AppendSolarArchetype(solarArch);
					}
				}

				if (ini.is_header("Trigger"))
				{
					TriggerArchetype trigger;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
						{
							trigger.name = ini.get_value_string(0);
						}
						else if (ini.is_value("InitState"))
						{
							trigger.active = ToLower(std::string(ini.get_value_string(0))) == "active";
						}
						else if (ini.is_value("Cnd_True"))
						{
							trigger.condition.first = TriggerCondition::Cnd_True;
							trigger.condition.second = NULL;
						}
						else if (ini.is_value("Cnd_Destroyed"))
						{
							trigger.condition.first = TriggerCondition::Cnd_Destroyed;
							std::shared_ptr<CndDestroyedArchetype> archetype(new CndDestroyedArchetype());
							archetype->objNameLabel = ini.get_value_string(0);
							archetype->count = ini.get_value_int(1);
							const std::string val = ToLower(ini.get_value_string(2));
							if (val == "explode")
								archetype->condition = DestroyedCondition::EXPLODE;
							else if (val == "silent")
								archetype->condition = DestroyedCondition::SILENT;
							else
								archetype->condition = DestroyedCondition::ALL;
							archetype->killerNameLabel = ini.get_value_string(3);
							trigger.condition.second = archetype;
						}
						else if (ini.is_value("Act_ActTrig"))
						{
							TriggerArchActionEntry action;
							action.first = TriggerAction::Act_ActTrig;
							std::shared_ptr<ActActTriggerArchetype> archetype(new ActActTriggerArchetype());
							archetype->triggerName = ini.get_value_string(0);
							archetype->activate = true;
							action.second = archetype;
							trigger.actions.push_back(action);
						}
						else if (ini.is_value("Act_DeactTrig"))
						{
							TriggerArchActionEntry action;
							action.first = TriggerAction::Act_ActTrig;
							std::shared_ptr<ActActTriggerArchetype> archetype(new ActActTriggerArchetype());
							archetype->triggerName = ini.get_value_string(0);
							archetype->activate = false;
							action.second = archetype;
							trigger.actions.push_back(action);
						}
						else if (ini.is_value("Act_LightFuse"))
						{
							TriggerArchActionEntry action;
							action.first = TriggerAction::Act_LightFuse;
							std::shared_ptr<ActLightFuseArchetype> archetype(new ActLightFuseArchetype());
							archetype->objName = mission.name + ":" + std::string(ini.get_value_string(0));
							archetype->fuseId = CreateID(ini.get_value_string(1));
							action.second = archetype;
							trigger.actions.push_back(action);
						}
						else if (ini.is_value("Act_ChangeState"))
						{
							TriggerArchActionEntry action;
							action.first = TriggerAction::Act_ChangeState;
							std::shared_ptr<ActChangeStateArchetype> archetype(new ActChangeStateArchetype());
							const std::string val = ToLower(ini.get_value_string(0));
							if (val == "succeed")
								archetype->state = MissionState::SUCCEED;
							else
								archetype->state = MissionState::FAIL;
							archetype->failTextId = ini.get_value_int(1);
							action.second = archetype;
							trigger.actions.push_back(action);
						}
						else if (ini.is_value("Act_SpawnSolar"))
						{
							TriggerArchActionEntry action;
							action.first = TriggerAction::Act_SpawnSolar;
							std::shared_ptr<ActSpawnSolarArchetype> archetype(new ActSpawnSolarArchetype());
							archetype->solarName = mission.name + ":" + std::string(ini.get_value_string(0));
							action.second = archetype;
							trigger.actions.push_back(action);
						}
						else if (ini.is_value("Act_Destroy"))
						{
							TriggerArchActionEntry action;
							action.first = TriggerAction::Act_Destroy;
							std::shared_ptr<ActDestroyArchetype> archetype(new ActDestroyArchetype());
							archetype->objName = mission.name + ":" + std::string(ini.get_value_string(0));
							const std::string val = ToLower(ini.get_value_string(1));
							if (val == "explode")
								archetype->destroyType = DestroyType::EXPLODE;
							else
								archetype->destroyType = DestroyType::VANISH;
							action.second = archetype;
							trigger.actions.push_back(action);
						}
					}
					mission.triggers.push_back(trigger);
				}
			}
			ini.close();

			if (!mission.name.empty())
				missionArchetypesByName[mission.name] = mission;
		}
	}

	static bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		for (const auto& missionArchetypeWithName: missionArchetypesByName)
		{
			if (missionArchetypeWithName.second.active)
				activeMissions.push_back(new Mission(missionArchetypeWithName.second));
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
 
		for (const auto& labelAndMission : activeMissions)
		{
			for (auto it = labelAndMission->objects.begin(); it != labelAndMission->objects.end();)
			{
				if (it->id == killedObject->cobj->id)
				{
					it = labelAndMission->objects.erase(it);
				}
				else
				{
					it++;
				}
			}
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
			if (missionArchetypesByName.contains(targetNickname))
			{
				activeMissions.push_back(new Mission(missionArchetypesByName[targetNickname]));
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}
			return false;
		}

		if (IS_CMD("stop_mission"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			for (auto it = activeMissions.begin(); it != activeMissions.end(); it++)
			{
				const auto mission = *it;
				if (mission->name == targetNickname)
				{
					TriggerArchetype triggerArch;
					triggerArch.name = "Manual Abort";

					std::shared_ptr<ActChangeStateArchetype> actChangeArchetype(new ActChangeStateArchetype());
					actChangeArchetype->state = MissionState::ABORT;
					triggerArch.actions.push_back({ TriggerAction::Act_ChangeState, actChangeArchetype });

					for (const auto& object : mission->objects)
					{
						std::shared_ptr<ActDestroyArchetype> actDestroyArchetype(new ActDestroyArchetype());
						actDestroyArchetype->destroyType = DestroyType::VANISH;
						actDestroyArchetype->objName = object.name;
						triggerArch.actions.push_back({ TriggerAction::Act_Destroy, actDestroyArchetype });
					}

					Trigger* abortionTrigger = new Trigger(mission, triggerArch);
					abortionTrigger->QueueExecution();
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return true;
				}
			}
			return false;
		}
		return false;
	}
}