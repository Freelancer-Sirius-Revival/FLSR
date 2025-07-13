#include "Formations.h"

namespace Formations
{
    std::unordered_map<uint, std::vector<Vector>> formationsById;

    std::vector<Vector> GetFormation(const uint id)
    {
        const auto& formationEntry = formationsById.find(id);
        if (formationEntry == formationsById.end())
        {
            ConPrint(L"ERROR: Formation does not exist: " + std::to_wstring(id) + L"\n");
            return std::vector<Vector>();
        }
        return formationEntry->second;
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

        if (ini.open((dataPath + "\\missions\\formations.ini").c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("Formation"))
                {
                    std::pair<uint, std::vector<Vector>> formation;
                    formation.first = 0;

                    while (ini.read_value())
                    {
                        if (ini.is_value("nickname"))
                        {
                            formation.first = CreateID(ini.get_value_string(0));
                        }
                        else if (ini.is_value("pos"))
                        {
                            formation.second.push_back(ini.get_vector());
                        }
                    }

                    if (formation.first)
                        formationsById.emplace(formation);
                }
            }
            ini.close();
        }
    }
}