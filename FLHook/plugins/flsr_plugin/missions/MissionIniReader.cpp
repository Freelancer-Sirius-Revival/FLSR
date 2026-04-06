#include "MissionIniReader.h"
#include "objectives/ObjIniReader.h"
#include "../SolarSpawn.h"

namespace Missions
{
	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void PrintMissionErrorToConsole(const std::wstring& entryName, const std::string missionName, const std::wstring& error)
	{
		ConPrint(L"ERROR: " + entryName + L" of Msn:" + stows(missionName) + L" " + error + L"\n");
	}

	bool TryReadMissionHeadFromIni(const uint nextMissionId, INI_Reader& ini)
	{
		if (!ini.is_header("Mission"))
			return false;
		
		const uint beginning = ini.tell();
		std::string name = "";
		while (ini.read_value())
		{
			if (ini.is_value("nickname"))
			{
				name = ToLower(ini.get_value_string(0));
				break;
			}
		}
		if (name.empty())
		{
			ConPrint(L"ERROR: Mission without nickname. Aborting!\n");
			return false;
		}

		ini.seek(beginning);
		bool initiallyActive = false;
		MissionOffer offer;
		while (ini.read_value())
		{
			if (ini.is_value("InitState"))
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
				{
					PrintMissionErrorToConsole(L"Mission", name, L"Invalid offer type. Defaulting to NONE.");
					offer.type = pub::GF::MissionType::Unknown;
				}
			}
			else if (ini.is_value("offer_target_system"))
				offer.system = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("offer_faction"))
				pub::Reputation::GetReputationGroup(offer.group, ini.get_value_string(0));
			else if (ini.is_value("offer_title_id"))
				offer.title = ini.get_value_int(0);
			else if (ini.is_value("offer_description_id"))
				offer.description = FmtStr(ini.get_value_int(0), 0);
			else if (ini.is_value("offer_reward"))
				offer.reward = std::max<uint>(0, ini.get_value_int(0));
			else if (ini.is_value("offer_ship_restriction"))
			{
				for (int index = 0, len = ini.get_num_parameters(); index < len; index++)
					offer.shipArchetypeIds.insert(CreateIdOrNull(ini.get_value_string(index)));
			}
			else if (ini.is_value("offer_bases"))
			{
				for (int index = 0, len = ini.get_num_parameters(); index < len; index++)
					offer.baseIds.insert(CreateIdOrNull(ini.get_value_string(index)));
			}
			else if (ini.is_value("reoffer"))
			{
				const auto value = ToLower(ini.get_value_string(0));
				if (value == "always")
					offer.reofferCondition = MissionReofferCondition::Always;
				else if (value == "onfail")
					offer.reofferCondition = MissionReofferCondition::OnFail;
				else if (value == "onsuccess")
					offer.reofferCondition = MissionReofferCondition::OnSuccess;
				else
				{
					PrintMissionErrorToConsole(L"Mission", name, L"Invalid offer reoffer type. Defaulting to NEVER.");
					offer.reofferCondition = MissionReofferCondition::Never;
				}
			}
			else if (ini.is_value("reoffer_delay"))
				offer.reofferDelay = std::max<float>(0.0f, ini.get_value_float(0));
		}
		if (offer.type != pub::GF::MissionType::Unknown)
		{
			// Never automatically start missions which are offered on the mission board.
			initiallyActive = false;

			if (!offer.system)
			{
				PrintMissionErrorToConsole(L"Mission", name, L"No offer target system. Aborting!");
				return false;
			}
			if (!offer.group)
			{
				PrintMissionErrorToConsole(L"Mission", name, L"Invalid offer faction. Aborting!");
				return false;
			}
			if (!offer.title)
			{
				PrintMissionErrorToConsole(L"Mission", name, L"No offer title. Aborting!");
				return false;
			}
			if (!offer.baseIds.empty())
			{
				PrintMissionErrorToConsole(L"Mission", name, L"No offer bases. Aborting!");
				return false;
			}
		}

		if (!missions.try_emplace(nextMissionId, name, nextMissionId, initiallyActive, true).second)
		{
			PrintMissionErrorToConsole(L"Mission", name, L"ID already existing. Aborting!");
			return false;
		}
		missions.at(nextMissionId).offer = offer;
		return true;
	}

	static void PrintErrorToConsole(const std::wstring& entryName, const Mission& mission, const std::wstring& error)
	{
		ConPrint(L"ERROR: " + entryName + L" of Msn:" + stows(mission.name) + L" " + error + L"\n");
	}

	bool TryReadMsnFormationFromIni(Mission& mission, INI_Reader& ini)
	{
		if (!ini.is_header("MsnFormation"))
			return false;

		MsnFormation formation;
		while (ini.read_value())
		{
			if (ini.is_value("nickname"))
				formation.id = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("position") && ini.get_num_parameters() == 3)
				formation.position = ini.get_vector();
			else if (ini.is_value("rotate") && ini.get_num_parameters() == 3)
				formation.rotation = ini.get_vector();
			else if (ini.is_value("formation"))
				formation.formationId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("ship"))
				formation.msnShipIds.push_back(CreateIdOrNull(ini.get_value_string(0)));
		}

		if (!formation.id)
		{
			PrintErrorToConsole(L"MsnFormation", mission, L"No nickname. Aborting!");
			return false;
		}
		if (!formation.formationId)
		{
			PrintErrorToConsole(L"MsnFormation", mission, L"No formation. Aborting!");
			return false;
		}
		if (formation.msnShipIds.empty())
		{
			PrintErrorToConsole(L"MsnFormation", mission, L"No ships. Aborting!");
			return false;
		}
		if (!mission.formations.insert({ formation.id, formation }).second)
		{
			PrintErrorToConsole(L"MsnFormation", mission, L"Nickname already existing. Aborting!");
			return false;
		}
		return true;
	}

	bool TryReadMsnSolarFromIni(Mission& mission, INI_Reader& ini)
	{
		if (!ini.is_header("MsnSolar"))
			return false;

		MsnSolar solar;
		while (ini.read_value())
		{
			if (ini.is_value("nickname"))
				solar.name = ToLower(ini.get_value_string(0));
			else if (ini.is_value("string_id"))
				solar.idsName = ini.get_value_int(0);
			else if (ini.is_value("system"))
				solar.systemId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("position") && ini.get_num_parameters() == 3)
				solar.position = ini.get_vector();
			else if (ini.is_value("rotate") && ini.get_num_parameters() == 3)
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

		if (solar.name.empty())
		{
			PrintErrorToConsole(L"MsnSolar", mission, L"No nickname. Aborting!");
			return false;
		}
		if (!solar.archetypeId)
		{
			PrintErrorToConsole(L"MsnSolar", mission, L"No archetype. Aborting!");
			return false;
		}
		if (!solar.systemId)
		{
			PrintErrorToConsole(L"MsnSolar", mission, L"No system. Aborting!");
			return false;
		}

		mission.solars.push_back(solar);
		SolarSpawn::SolarArchetype solarArch;
		solarArch.archetypeId = solar.archetypeId;
		solarArch.loadoutId = solar.loadoutId;
		solarArch.nickname = mission.name + ":" + solar.name;
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
		return true;
	}

	bool TryReadNpcFromIni(Mission& mission, INI_Reader& ini)
	{
		if (!ini.is_header("Npc"))
			return false;

		Npc npc;
		while (ini.read_value())
		{
			if (ini.is_value("nickname"))
				npc.id = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("archetype"))
				npc.archetypeId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("loadout"))
				npc.loadoutId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("state_graph"))
				npc.stateGraph = ToLower(ini.get_value_string(0));
			else if (ini.is_value("faction"))
				npc.faction = ini.get_value_string(0);
			else if (ini.is_value("pilot"))
				npc.pilotId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("level"))
				npc.level = ini.get_value_int(0);
		}

		if (!npc.id)
		{
			PrintErrorToConsole(L"Npc", mission, L"No nickname. Aborting!");
			return false;
		}
		if (!npc.archetypeId)
		{
			PrintErrorToConsole(L"Npc", mission, L"No archetype. Aborting!");
			return false;
		}
		if (!npc.loadoutId)
		{
			PrintErrorToConsole(L"Npc", mission, L"No loadout. Aborting!");
			return false;
		}
		if (npc.stateGraph.empty())
		{
			PrintErrorToConsole(L"Npc", mission, L"No state graph. Aborting!");
			return false;
		}
		if (!npc.pilotId)
		{
			PrintErrorToConsole(L"Npc", mission, L"No pilot. Aborting!");
			return false;
		}
		if (npc.faction.empty())
		{
			PrintErrorToConsole(L"Npc", mission, L"No faction. Aborting!");
			return false;
		}
		if (!mission.npcs.insert({ npc.id, npc }).second)
		{
			PrintErrorToConsole(L"Npc", mission, L"Nickname already existing. Aborting!");
			return false;
		}
		return true;
	}

	bool TryReadMsnNpcFromIni(Mission& mission, INI_Reader& ini)
	{
		if (!ini.is_header("MsnNpc"))
			return false;

		MsnNpc npc;
		while (ini.read_value())
		{
			if (ini.is_value("nickname"))
				npc.id = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("string_id"))
				npc.idsName = ini.get_value_int(0);
			else if (ini.is_value("use_ship_ids"))
				npc.shipNameDisplayed = ini.get_value_bool(0);
			else if (ini.is_value("system"))
				npc.systemId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("position") && ini.get_num_parameters() == 3)
				npc.position = ini.get_vector();
			else if (ini.is_value("rotate") && ini.get_num_parameters() == 3)
				npc.orientation = EulerMatrix(ini.get_vector());
			else if (ini.is_value("npc"))
				npc.npcId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("hitpoints"))
				npc.hitpoints = ini.get_value_int(0);
			else if (ini.is_value("voice"))
				npc.voiceId = CreateIdOrNull(ini.get_value_string(0));
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
			else if (ini.is_value("pilot_job"))
				npc.pilotJobId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("arrival_obj"))
				npc.startingObjId = CreateIdOrNull(ini.get_value_string(0));
			else if (ini.is_value("label"))
				npc.labels.insert(CreateIdOrNull(ini.get_value_string(0)));
		}

		if (!npc.id)
		{
			PrintErrorToConsole(L"MsnNpc", mission, L"No nickname. Aborting!");
			return false;
		}
		if (!npc.npcId)
		{
			PrintErrorToConsole(L"MsnNpc", mission, L"No NPC. Aborting!");
			return false;
		}
		if (!npc.systemId)
		{
			PrintErrorToConsole(L"MsnNpc", mission, L"No system. Aborting!");
			return false;
		}
		if (!mission.msnNpcs.insert({ npc.id, npc }).second)
		{
			PrintErrorToConsole(L"MsnNpc", mission, L"Nickname already existing. Aborting!");
			return false;
		}
		return true;
	}

	bool TryReadObjectiveListFromIni(Mission& mission, INI_Reader& ini)
	{
		if (!ini.is_header("ObjList"))
			return false;

		const uint beginning = ini.tell();
		uint nickname = 0;
		while (ini.read_value())
		{
			if (ini.is_value("nickname"))
			{
				nickname = CreateIdOrNull(ini.get_value_string(0));
				break;
			}
		}
		if (!nickname)
		{
			ConPrint(L"ERROR: ObjList of Msn:" + std::to_wstring(mission.id) + L" No nickname. Aborting!\n");
			return false;
		}

		ini.seek(beginning);
		const ObjectiveParent objParent(mission.id, nickname);
		Objectives objectives(objParent.objectivesId, objParent.missionId);
		while (ini.read_value())
		{
			const auto& obj = TryReadObjectiveFromIni(objParent, ini);
			if (obj != nullptr)
				objectives.objectives.push_back(ObjectivePtr(obj));
		}

		if (!mission.objectives.insert({ objectives.id, objectives }).second)
		{
			ConPrint(L"ERROR: ObjList of Msn:" + std::to_wstring(mission.id) + L" Nickname already existing. Aborting!\n");
			return false;
		}
		return true;
	}

	bool TryReadDialogFromIni(Mission& mission, INI_Reader& ini)
	{
		if (!ini.is_header("Dialog"))
			return false;

		Dialog dialog;
		while (ini.read_value())
		{
			if (ini.is_value("nickname"))
			{
				dialog.id = CreateIdOrNull(ini.get_value_string(0));
			}
			else if (ini.is_value("etherSender"))
			{
				DialogEtherSender sender;
				sender.id = CreateIdOrNull(ini.get_value_string(0));
				sender.voiceId = CreateIdOrNull(ini.get_value_string(1));
				sender.idsName = ini.get_value_int(2);
				sender.costume.head = CreateIdOrNull(ini.get_value_string(3));
				sender.costume.body = CreateIdOrNull(ini.get_value_string(4));
				int count = 0;
				while (count < 8)
				{
					const auto val = ini.get_value_string(5 + count);
					if (strlen(val) == 0)
						break;
					sender.costume.accessory[count++] = CreateIdOrNull(val);
				}
				sender.costume.accessories = count;

				if (!sender.id)
				{
					PrintErrorToConsole(L"Dialog", mission, L"EtherSender " + std::to_wstring(dialog.etherSenders.size() + 1) + L" without nickname. Aborting!");
					return false;
				}
				if (!sender.voiceId)
				{
					PrintErrorToConsole(L"Dialog", mission, L"EtherSender " + std::to_wstring(dialog.etherSenders.size() + 1) + L" without voice. Aborting!");
					return false;
				}
				if (!dialog.etherSenders.insert({ sender.id, sender }).second)
				{
					PrintErrorToConsole(L"Dialog", mission, L"EtherSender " + std::to_wstring(dialog.etherSenders.size() + 1) + L" nickname already existing. Aborting!");
					return false;
				}
			}
			else if (ini.is_value("line"))
			{
				DialogLine line;
				line.id = CreateIdOrNull(ini.get_value_string(0));
				line.receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
				line.senderEtherSenderOrObjName = CreateIdOrNull(ini.get_value_string(2));
				uint pos = 3;
				while (!ini.is_value_empty(pos))
				{
					const char* val = ini.get_value_string(pos);
					// Make sure we do not go beyond the following numeric value.
					char* end;
					strtol(val, &end, 10);
					if (end != val)
						break;
					line.lines.push_back(CreateIdOrNull(val));
					pos++;
				}
				if (!ini.is_value_empty(pos++))
					line.delay = ini.get_value_float(pos - 1);
				if (ini.get_num_parameters() > pos)
					line.global = ini.get_value_bool(pos);

				if (!line.id)
				{
					PrintErrorToConsole(L"Dialog", mission, L"Line " + std::to_wstring(dialog.lines.size() + 1) + L" without nickname. Aborting!");
					return false;
				}
				if (!line.senderEtherSenderOrObjName)
				{
					PrintErrorToConsole(L"Dialog", mission, L"Line " + std::to_wstring(dialog.lines.size() + 1) + L" without sender. Aborting!");
					return false;
				}
				if (line.lines.empty())
				{
					PrintErrorToConsole(L"Dialog", mission, L"Line " + std::to_wstring(dialog.lines.size() + 1) + L" without lines. Aborting!");
					return false;
				}
				dialog.lines.push_back(line);
			}
		}

		if (!dialog.id)
		{
			PrintErrorToConsole(L"Dialog", mission, L"No nickname. Aborting!");
			return false;
		}
		if (dialog.lines.empty())
		{
			PrintErrorToConsole(L"Dialog", mission, L"No lines. Aborting!");
			return false;
		}
		for (const auto& etherSender : dialog.etherSenders)
		{
			bool found = false;
			for (const auto& line : dialog.lines)
			{
				if (line.senderEtherSenderOrObjName == etherSender.second.id)
				{
					found = true;
					break;
				}
			}
			if (!found)
				PrintErrorToConsole(L"Dialog", mission, L"EitherSender " + std::to_wstring(etherSender.second.id) + L" not used.");
		}
		if (!mission.dialogs.insert({ dialog.id, dialog }).second)
		{
			PrintErrorToConsole(L"Dialog", mission, L"Nickname already existing. Aborting!");
			return false;
		}
		return true;
	}
}