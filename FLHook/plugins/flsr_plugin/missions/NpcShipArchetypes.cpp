#include "NpcShipArchetypes.h"

namespace NpcShipArchetypes
{
    std::unordered_map<uint, NpcShipArch> npcShipArchTypesById;

    bool GetNpcShipArch(const uint id, NpcShipArch& result)
    {
        const auto& npcShipArchEntry = npcShipArchTypesById.find(id);
        if (npcShipArchEntry == npcShipArchTypesById.end())
        {
            ConPrint(L"ERROR: NpcShipArch does not exist: " + std::to_wstring(id) + L"\n");
            return false;
        }
        result = npcShipArchEntry->second;
        return true;
    }

    static byte GetDifficulty(const char* str)
    {
        if (strlen(str) < 2)
            return 0;

        if (strchr(str, 'd') != str)
            return 0;

        const char* start = str + 1;
        char* end;
        const long diff = strtol(start, &end, 10);
        return start != end ? std::min<byte>(255, diff) : 0;
    }

    void ReadFiles()
    {
        std::string dataPath = "..\\data";;
        INI_Reader ini;
        if (ini.open("freelancer.ini", false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("Freelancer"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("data path"))
                        {
                            dataPath = ini.get_value_string(0);
                            break;
                        }
                    }
                    break;
                }
            }
            ini.close();
        }

        if (ini.open((dataPath + "\\missions\\npcships.ini").c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("NpcShipArch"))
                {
                    NpcShipArch npcShipArch;

                    while (ini.read_value())
                    {
                        if (ini.is_value("nickname"))
                        {
                            npcShipArch.id = CreateID(ini.get_value_string(0));
                        }
                        else if (ini.is_value("loadout"))
                        {
                            npcShipArch.loadoutId = CreateID(ini.get_value_string(0));
                        }
                        else if (ini.is_value("ship_archetype"))
                        {
                            npcShipArch.archetypeId = CreateID(ini.get_value_string(0));
                        }
                        else if (ini.is_value("state_graph"))
                        {
                            npcShipArch.stateGraph = ini.get_value_string(0);
                        }
                        else if (ini.is_value("pilot"))
                        {
                            npcShipArch.pilotId = CreateID(ini.get_value_string(0));
                        }
                        else if (ini.is_value("level"))
                        {
                            npcShipArch.level = GetDifficulty(ini.get_value_string(0));
                        }
                        else if (ini.is_value("npc_class"))
                        {
                            for (int i = 0, length = ini.get_num_parameters(); i < length; i++)
                            {
                                const auto value = ini.get_value_string(i);
                                npcShipArch.classes.insert(CreateID(value));
                                const auto diff = GetDifficulty(value);
                                if (diff > 0)
                                    npcShipArch.difficulties.insert(diff);
                            }
                        }
                    }

                    if (npcShipArch.id != 0 && npcShipArch.loadoutId != 0 && npcShipArch.archetypeId != 0 && !npcShipArch.stateGraph.empty() && npcShipArch.pilotId != 0)
                        npcShipArchTypesById.try_emplace(npcShipArch.id, npcShipArch);
                }
            }
            ini.close();
        }
    }
}