#pragma once
#include "NpcShips.h"

namespace RandomMissions
{
	std::unordered_map<uint, NpcShip> npcShipById;

	void ReadNpcShipData() {
		INI_Reader ini;
		if (!ini.open("..\\DATA\\MISSIONS\\npcships.ini", false)) return;

		while (ini.read_header())
		{
			if (ini.is_header("NPCShipArch"))
			{
				NpcShip ship;
				bool missionValid = false;
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						ship.id = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("level"))
					{
						const char* val = ini.get_value_string(0) + 1;
						char* end;
						ship.level = strtol(val, &end, 10);
					}
					else if (ini.is_value("loadout"))
					{
						ship.loadoutId = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("ship_archetype"))
					{
						ship.archetypeId = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("state_graph"))
					{
						ship.stateGraph = ini.get_value_string(0);
					}
					else if (ini.is_value("pilot"))
					{
						ship.pilotId = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("npc_class"))
					{
						for (int i = 0, length = ini.get_num_parameters(); i < length; i++)
						{
							const char* val = ini.get_value_string(0) + 1;
							missionValid = missionValid || std::string(val) == "class_mission";
							char* end;
							long diff = strtol(val, &end, 10);
							if (val != end)
							{
								ship.difficulties.insert(diff);
							}
						}
					}
				}
				if (missionValid)
				{
					npcShipById.insert({ ship.id, ship });
				}
			}
		}
		ini.close();
	}
}