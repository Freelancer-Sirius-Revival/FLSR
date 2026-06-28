#include <regex>
#include <filesystem>
#include "Missions.h"
#include "MissionIniReader.h"
#include "actions/ActIniReader.h"
#include "conditions/CndTrue.h"
#include "conditions/CndIniReader.h"
#include "MissionBoard.h"
#include "ClientObjectives.h"
#include "actions/ActActMsnTrig.h"
#include "actions/ActActTrig.h"

namespace Missions
{	
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
		uint nextMissionId = 0;

		const std::regex filePattern(".+\\.ini", std::regex_constants::ECMAScript | std::regex_constants::icase);
		for (const auto& entry : std::filesystem::recursive_directory_iterator(missionDirectory))
		{
			const std::string fileName = wstos(entry.path().filename());
			INI_Reader ini;
			if (std::regex_match(fileName, filePattern) && ini.open((missionDirectory + fileName).c_str(), false))
			{
				uint missionId = 0;
				while (ini.read_header())
				{
					if (TryReadMissionHeadFromIni(nextMissionId, ini))
					{
						missionId = nextMissionId;
						nextMissionId++;
					}
					if (missions.empty())
						continue;
					Mission& mission = missions.at(missionId);
					TryReadMsnFormationFromIni(mission, ini);
					TryReadNpcFromIni(mission, ini);
					TryReadMsnNpcFromIni(mission, ini);
					TryReadMsnSolarFromIni(mission, ini);
					TryReadDialogFromIni(mission, ini);
					TryReadObjectiveListFromIni(mission, ini);

					if (ini.is_header("Trigger"))
					{
						std::string name = "";
						uint id = 0;
						bool initiallyActive = false;
						Trigger::TriggerRepeatable repeatable = Trigger::TriggerRepeatable::Off;
						ConditionPtr condition = nullptr;
						std::vector<ActionPtr> actions;

						const auto start = ini.tell();
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
							{
								name = ini.get_value_string(0);
								id = CreateIdOrNull(name.c_str());
								break;
							}
						}
						ini.seek(start);

						const ConditionParent conditionParent(missionId, id);

						while (id && ini.read_value())
						{
							if (ini.is_value("InitState"))
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
							else
							{
								if (const auto& act = TryReadActionFromIni(ini); act)
								{
									actions.push_back(ActionPtr(act));
								}
								else if (const auto& cnd = TryReadConditionFromIni(conditionParent, ini); cnd)
								{
									if (condition != nullptr)
										ConPrint(L"Trigger " + std::to_wstring(id) + L" already has a condition! Overriding.");
									condition = ConditionPtr(cnd);
								}
							}
						}

						if (id)
						{
							missions.at(missionId).triggers.try_emplace(id, name, id, missionId, initiallyActive, repeatable);
							Trigger& trigger = missions.at(missionId).triggers.at(id);
							if (condition == nullptr)
								ConPrint(L"Trigger " + std::to_wstring(id) + L" has no condition! Falling back to Cnd_True\n");
							trigger.condition = condition != nullptr ? condition : ConditionPtr(new CndTrue({ missionId, id }));
							trigger.actions = actions;
						}
					}
				}
				ini.close();
			}
		}
	}
	
	void StartMissionByOfferId(const uint offerId, const uint startingClientId, const std::vector<uint>& clientIds)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.offerId == offerId && missionEntry.second.CanBeStarted())
			{
				auto& mission = missionEntry.second;
				mission.AddLabelToObject(MissionObject(MissionObjectType::Client, startingClientId), CreateID("initial_player"));
				const uint labelId = CreateID("players");
				for (const auto clientId : clientIds)
					mission.AddLabelToObject(MissionObject(MissionObjectType::Client, clientId), labelId);
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

		ConPrint(L"Initializing Missions...\n");

		LoadSettings();

		ConPrint(L"Starting initial missions...\n");

		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.initiallyActive)
				missionEntry.second.Start();
		}

		ConPrint(L"Done initializing Missions\n");
	}

	static void ClearMissions()
	{
		auto it = missions.begin();
		while (it != missions.end())
		{
			MissionBoard::DeleteOffer(it->second.offerId);
			it = missions.erase(it);
		}
	}

	void __stdcall Shutdown()
	{
		ClearMissions();
		returncode = DEFAULT_RETURNCODE;
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

	static void RemoveClientFromMissions(const uint clientId)
	{
		ClientObjectives::ClearClientObjectives(clientId, 0);
		// Removing a client from a mission could potentially end and remove it. So first gather the mission IDs and then delete them one by one.
		std::vector<uint> ids;
		for (const auto& entry : missions)
			ids.push_back(entry.first);

		for (const auto id : ids)
		{
			if (const auto& entry = missions.find(id); entry != missions.end())
				entry->second.RemoveClient(clientId);
		}

		// Remove all mission cargo to avoid it being left-over in the cargo hold.
		for (const auto& equip : Players[clientId].equipDescList.equip)
		{
			if (equip.bMission)
				pub::Player::RemoveCargo(clientId, equip.sID, equip.iCount);
		}
	}

	void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		RemoveObjectFromMissions(killedObject->cobj->id);
		returncode = DEFAULT_RETURNCODE;
	}

	std::unordered_map<uint, CHARACTER_ID> lastCharacterByClientId;

	void __stdcall CharacterSelect(const CHARACTER_ID& cId, unsigned int clientId)
	{
		const auto& foundEntry = lastCharacterByClientId.find(clientId);
		if (foundEntry == lastCharacterByClientId.end() || !(foundEntry->second == cId))
			RemoveClientFromMissions(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall CharacterSelect_AFTER(const CHARACTER_ID& cId, unsigned int clientId)
	{
		lastCharacterByClientId[clientId] = cId;
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2)
	{
		lastCharacterByClientId.erase(clientId);
		RemoveClientFromMissions(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	static bool StartMission(const std::string& missionName)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.name == missionName && !missionEntry.second.IsActive())
			{
				missionEntry.second.Start();	
				return true;
			}
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

	#define IS_CMD(a) !wscCmd.compare(L##a)

	bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd)
	{
		if (IS_CMD("start_mission"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (StartMission(targetNickname))
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" started.");
			else
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" not found or already running.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		else if (IS_CMD("stop_mission"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (EndMission(targetNickname))
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" ended.");
			else
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" not found or already stopped.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		else if (IS_CMD("reload_missions"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			ClearMissions();
			initialized = false;
			PrintUserCmdText(clientId, L"Stopped and reloaded all missions.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		else if (IS_CMD("act_trigger"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string msnNickname = wstos(ToLower(cmds->ArgStr(1)));
			const std::string trigNickname = wstos(ToLower(cmds->ArgStr(2)));
			ActActMsnTrig action;
			action.missionId = CreateIdOrNull(msnNickname.c_str());
			ActActTrigEntry entry;
			entry.triggerId = CreateIdOrNull(trigNickname.c_str());
			action.triggers.push_back(entry);
			action.activate = true;
			Mission msn("", 0, false, false);
			action.Execute(msn, MissionObject(MissionObjectType::Client, clientId));

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		else if (IS_CMD("deact_trigger"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string msnNickname = wstos(ToLower(cmds->ArgStr(1)));
			const std::string trigNickname = wstos(ToLower(cmds->ArgStr(2)));
			ActActMsnTrig action;
			action.missionId = CreateIdOrNull(msnNickname.c_str());
			ActActTrigEntry entry;
			entry.triggerId = CreateIdOrNull(trigNickname.c_str());
			action.triggers.push_back(entry);
			action.activate = true;
			Mission msn("", 0, false, false);
			action.Execute(msn, MissionObject(MissionObjectType::Client, clientId));

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		else if (IS_CMD("getpos"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_SUPERADMIN))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			IObjRW* inspect;
			StarSystem* system;
			if (shipId && GetShipInspect(shipId, inspect, system))
				PrintUserCmdText(clientId, L"Pos: " + std::to_wstring(inspect->cobj->vPos.x) + L", " + std::to_wstring(inspect->cobj->vPos.y) + L", " + std::to_wstring(inspect->cobj->vPos.z));
			else
				PrintUserCmdText(clientId, L"You must be in space for that.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		returncode = DEFAULT_RETURNCODE;
		return false;
	}
}