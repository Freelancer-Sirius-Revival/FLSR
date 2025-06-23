#include "BestPath.h"

namespace BestPath
{
    std::unordered_map<uint, std::unordered_map<uint, uint>> systemsFromIdToIdNextId;
    std::unordered_map<uint, std::unordered_map<uint, XRequestBestPathEntry>> gatesToNextSystemIdPerStartSystemId;
    std::unordered_map<uint, std::unordered_map<uint, XRequestBestPathEntry>> holesToNextSystemIdPerStartSystemId;

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

    bool initialized = false;
    void CollectJumpObjectsPerSystem()
    {
        if (initialized)
            return;
        initialized = true;

        CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
        while (solar != NULL)
        {
            const uint type = solar->get_type();
            if (type == ObjectType::JumpGate)
            {
                XRequestBestPathEntry entry;
                entry.position = solar->vPos;
                entry.objId = solar->id;
                entry.systemId = solar->system;
                gatesToNextSystemIdPerStartSystemId[solar->system].try_emplace(solar->jumpDestSystem, entry);
            }
            else if (type == ObjectType::JumpHole)
            {
                XRequestBestPathEntry entry;
                entry.position = solar->vPos;
                entry.objId = solar->id;
                entry.systemId = solar->system;
                holesToNextSystemIdPerStartSystemId[solar->system].try_emplace(solar->jumpDestSystem, entry);
            }
            solar = static_cast<CSolar*>(solar->FindNext());
        }
    }

    XRequestBestPathEntry GetJumpObjectToNextSystem(const uint clientId, const uint targetSystem) // TODO go by Reputation (for locked gates)
	{
        XRequestBestPathEntry result;
        result.objId = 0;
        result.systemId = 0;
        result.position = Vector(0, 0, 0);

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

        const uint nextSystemId = targetEntry->second;

        const auto& gateStartSystemEntry = gatesToNextSystemIdPerStartSystemId.find(startSystemId);
        if (gateStartSystemEntry != gatesToNextSystemIdPerStartSystemId.end())
        {
            const auto& gateNextSystemEntry = gateStartSystemEntry->second.find(nextSystemId);
            if (gateNextSystemEntry != gateStartSystemEntry->second.end())
                return gateNextSystemEntry->second;
        }

        const auto& holeStartSystemEntry = holesToNextSystemIdPerStartSystemId.find(startSystemId);
        if (holeStartSystemEntry != holesToNextSystemIdPerStartSystemId.end())
        {
            const auto& holeNextSystemEntry = holeStartSystemEntry->second.find(nextSystemId);
            if (holeNextSystemEntry != holeStartSystemEntry->second.end())
                return holeNextSystemEntry->second;
        }
        return result;
	}
}