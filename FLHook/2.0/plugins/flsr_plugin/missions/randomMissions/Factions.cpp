#pragma once
#include "Factions.h"
#include "Costumes.h"

namespace RandomMissions
{
	std::unordered_map<uint, Faction> factionById;

	void ReadFactionData() {
		INI_Reader ini;

		if (!ini.open("..\\DATA\\initialworld.ini", false)) return;

		std::unordered_map<uint, std::unordered_set<uint>> hostileFactionsByFactionId;
		while (ini.read_header())
		{
			if (ini.is_header("Group"))
			{
				uint factionId = 0;
				std::unordered_set<uint> hostileFactions;
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						factionId = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("rep"))
					{
						if (ini.get_value_float(0) <= -0.6f)
						{
							hostileFactions.insert(CreateID(ini.get_value_string(1)));
						}
					}
				}
				if (factionId)
				{
					hostileFactionsByFactionId[factionId] = hostileFactions;
				}
			}
		}


		if (!ini.open("..\\DATA\\MISSIONS\\faction_prop.ini", false)) return;

		while (ini.read_header())
		{
			if (ini.is_header("FactionProps"))
			{
				Faction faction;
				while (ini.read_value())
				{
					if (ini.is_value("affiliation"))
					{
						faction.id = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("npc_ship"))
					{
						faction.npcShipIds.insert(CreateID(ini.get_value_string(0)));
					}
					else if (ini.is_value("voice"))
					{
						faction.maleVoiceIds.insert(CreateID(ini.get_value_string(0)));
						//TODO separate female voices
					}
					else if (ini.is_value("mc_costume"))
					{
						faction.missionCommission = costumeById[CreateID(ini.get_value_string(0))];
					}
				}
				if (faction.id)
				{
					faction.hostileFactionIds = hostileFactionsByFactionId[faction.id];
					factionById[faction.id] = faction;
				}
			}
		}
		ini.close();
	}
}