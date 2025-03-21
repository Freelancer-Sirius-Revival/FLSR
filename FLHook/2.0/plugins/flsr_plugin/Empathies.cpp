#include "Empathies.h"
#include <FLHook.h>

namespace Empathies
{
    struct RepChangeEffects
    {
        uint groupId = 0;
        float objectDestruction = 0;
        std::unordered_map<uint, float> empathyRates;
    };

    std::unordered_map<uint, RepChangeEffects> groupReputationChangeEffects;

    float maxReputationThreshold = 0.9f;

	static void ChangeReputationRelative(const int reputationId, const int groupId, const float change)
	{
		if (change == 0.0f)
			return;

		float currentValue;
		pub::Reputation::GetGroupFeelingsTowards(reputationId, groupId, currentValue);
		pub::Reputation::SetReputation(reputationId, groupId, std::max<float>(-maxReputationThreshold, std::min<float>(currentValue + change, maxReputationThreshold)));
	}

    void ChangeReputationsByObjectDestruction(const unsigned int clientId, const unsigned int groupId)
	{
        const auto& reputationChangeEffectsEntry = groupReputationChangeEffects.find(groupId);
        if (reputationChangeEffectsEntry == groupReputationChangeEffects.end())
            return;
		const auto& reputationChangeEffects = reputationChangeEffectsEntry->second;

		int reputationId;
		pub::Player::GetRep(clientId, reputationId);

		// Add direct reputation change of the destroyed object.
		ChangeReputationRelative(reputationId, groupId, reputationChangeEffects.objectDestruction);

		// Add indirect reputation changes of the destroyed object.
		for (const auto& empathyRate : reputationChangeEffects.empathyRates)
			ChangeReputationRelative(reputationId, empathyRate.first, reputationChangeEffects.objectDestruction * empathyRate.second);
	}

	static HMODULE LoadContentDll()
	{
		INI_Reader ini;
		if (ini.open("freelancer.ini", false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Initial MP DLLs"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("path"))
						{
							const std::string dataPath = ini.get_value_string(0);
							return GetModuleHandle((dataPath + "\\Content.dll").c_str());
						}
					}
					break;
				}
			}
			ini.close();
		}
		return 0;
	}

	static void ReadFiles()
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

        if (ini.open((dataPath + "\\missions\\empathy.ini").c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("RepChangeEffects"))
                {
                    RepChangeEffects repChange;
                    while (ini.read_value())
                    {
                        if (ini.is_value("group"))
                        {
                            uint groupId;
                            pub::Reputation::GetReputationGroup(groupId, ini.get_value_string(0));
                            repChange.groupId = groupId;
                        }

                        if (ini.is_value("event") && std::string(ini.get_value_string(0)) == "object_destruction")
                            repChange.objectDestruction = ini.get_value_float(1);

                        if (ini.is_value("empathy_rate"))
                        {
                            uint groupId;
                            pub::Reputation::GetReputationGroup(groupId, ini.get_value_string(0));
                            repChange.empathyRates.insert({ groupId, ini.get_value_float(1) });
                        }
                    }
                    if (repChange.groupId)
                    {
                        repChange.empathyRates.erase(repChange.groupId); // Make sure the faction isn't modifying itself.
                        groupReputationChangeEffects[repChange.groupId] = repChange;
                    }
                }
            }
            ini.close();
        }
	}

    bool initialized = false;
    void Initialize()
    {
        if (initialized)
            return;
        initialized = true;

        ReadFiles(); // Requires Reputations to have been loaded.

        const HMODULE contentHandle = LoadContentDll();
        if (contentHandle)
            maxReputationThreshold = 1.0f - static_cast<float>(*(double*)(DWORD(contentHandle) + 0x11B930));
    }
}