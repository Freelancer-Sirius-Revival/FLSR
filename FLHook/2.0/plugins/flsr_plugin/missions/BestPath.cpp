#include "BestPath.h"

namespace BestPath
{
    std::unordered_map<uint, std::unordered_map<uint, uint>> systemsFromIdToIdNextId;

	void ReadFiles()
	{
        char currentDirectory[MAX_PATH];
        GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);

        INI_Reader ini;
        std::string systemsShortestPathFilePath = "";
        if (ini.open("freelancer.ini", false))
        {
            while (ini.read_header() && systemsShortestPathFilePath.empty())
            {
                if (ini.is_header("Freelancer"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("data path"))
                        {
                            systemsShortestPathFilePath = ini.get_value_string(0);
                            systemsShortestPathFilePath += "\\UNIVERSE\\systems_shortest_path.ini";
                            break;
                        }
                    }
                }
            }
            ini.close();
        }

        if (!systemsShortestPathFilePath.empty() && ini.open(systemsShortestPathFilePath.c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("SystemConnections"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("Path"))
                        {
                            const uint startSystemId = CreateID(ini.get_value_string(0));
                            const uint targetSystemId = CreateID(ini.get_value_string(1));
                            const uint nextSystemId = startSystemId == targetSystemId ? startSystemId : CreateID(ini.get_value_string(3));
                            systemsFromIdToIdNextId[startSystemId][targetSystemId] = nextSystemId;
                        }
                    }
                }
            }
            ini.close();
        }
	}

    BestPathNextObject GetJumpObjectToNextSystem(const uint clientId, const uint targetSystem)
	{
        BestPathNextObject result;
        result.objId = 0;

        if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return result;

        uint startSystemId;
        pub::Player::GetSystem(clientId, startSystemId);
        const auto& fromEntry = systemsFromIdToIdNextId.find(startSystemId);
        if (fromEntry == systemsFromIdToIdNextId.end())
            return result;

        const auto& targetEntry = fromEntry->second.find(targetSystem);
        if (targetEntry == fromEntry->second.end())
            return result;

        const uint nextSystem = targetEntry->second;
        const auto& system = SysDB::SysMap.find(startSystemId);
        if (system == SysDB::SysMap.end())
            return result;

        CSolar* gate = nullptr;
        CSolar* hole = nullptr;

        const auto& solarList = (*system).second.solarList;
        for (auto obj = solarList.start; obj != solarList.end; obj = obj->next)
        {
            if (obj->value->cobj->type & (ObjectType::JumpGate | ObjectType::JumpHole))
            {
                const auto solar = dynamic_cast<CSolar*>(obj->value->cobj);
                if (solar->jumpDestSystem == nextSystem)
                {
                    if (solar->type & ObjectType::JumpGate)
                        gate = solar;
                    else
                        hole = solar;
                }
            }
        }

        if (gate != nullptr)
        {
            result.objId = gate->id;
            result.position = gate->get_position();
        }
        if (hole != nullptr)
        {
            result.objId = hole->id;
            result.position = hole->get_position();
        }
        return result;
	}
}