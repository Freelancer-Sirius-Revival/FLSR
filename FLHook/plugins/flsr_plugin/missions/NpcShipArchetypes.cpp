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
                            std::string value = ini.get_value_string(0);
                            const auto dPos = value.find("d");
                            if (dPos != std::string::npos)
                                value = value.substr(dPos + 1);
                            npcShipArch.level = std::min<uint>(255, stoi(value, NULL, 0));
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