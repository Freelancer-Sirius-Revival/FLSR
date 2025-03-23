#include "NpcNames.h"
#include <FLHook.h>
#include <random>

namespace NpcNames
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	enum class Gender
	{
		Male,
		Female
	};

	static std::unordered_map<uint, Gender> voiceGenders;
	static std::unordered_map<uint, std::pair<uint, uint>> factionMaleFirstNames;
	static std::unordered_map<uint, std::pair<uint, uint>> factionFemaleFirstNames;
	static std::unordered_map<uint, std::pair<uint, uint>> factionLastNames;
	static std::unordered_map<uint, std::pair<uint, uint>> factionLargeShipNames;
	static std::unordered_map<uint, uint> factionLargeShipDesignations;

	std::pair<unsigned int, unsigned int> GetRandomName(const unsigned int factionId, const unsigned int voiceId)
	{
		std::pair<uint, uint> result = { 0, 0 };
		const auto& genderEntry = voiceGenders.find(voiceId);
		if (genderEntry == voiceGenders.end())
			return result;

		const auto& firstNames = genderEntry->second == Gender::Male ? factionMaleFirstNames : factionFemaleFirstNames;
		if (const auto& nameEntry = firstNames.find(factionId); nameEntry != firstNames.end())
			result.first = std::uniform_int_distribution(nameEntry->second.first, nameEntry->second.second)(gen);

		if (const auto& nameEntry = factionLastNames.find(factionId); nameEntry != factionLastNames.end())
			result.second = std::uniform_int_distribution(nameEntry->second.first, nameEntry->second.second)(gen);

		return result;
	}

	std::pair<unsigned int, unsigned int> GetRandomLargeShipName(const unsigned int factionId)
	{
		std::pair<uint, uint> result = { 0, 0 };

		if (const auto& nameEntry = factionLargeShipDesignations.find(factionId); nameEntry != factionLargeShipDesignations.end())
			result.first = nameEntry->second;

		if (const auto& nameEntry = factionLargeShipNames.find(factionId); nameEntry != factionLargeShipNames.end())
			result.second = std::uniform_int_distribution(nameEntry->second.first, nameEntry->second.second)(gen);

		return result;
	}

	void ReadFiles()
	{
		std::string dataPath = "..\\data";
		std::vector<std::string> voicePaths;
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
							dataPath = ini.get_value_string(0);
					}
				}
				else if (ini.is_header("Data"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("voices"))
							voicePaths.push_back(ini.get_value_string(0));
					}
				}
			}
			ini.close();
		}

		for (const auto& filePath : voicePaths)
		{
			if (ini.open((dataPath + "\\" + filePath).c_str(), false))
			{
				while (ini.read_header())
				{
					if (ini.is_header("mVoiceProp"))
					{
						uint voiceId = 0;
						Gender gender = Gender::Male;
						while (ini.read_value())
						{
							if (ini.is_value("voice"))
								voiceId = CreateID(ini.get_value_string(0));
							else if (ini.is_value("gender"))
								gender = ToLower(ini.get_value_string(0)) == "female" ? Gender::Female : Gender::Male;
						}
						if (voiceId)
							voiceGenders.insert({ voiceId, gender });
					}
				}
				ini.close();
			}
		}

		if (ini.open((dataPath + "\\missions\\faction_prop.ini").c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("FactionProps"))
				{
					uint factionId = 0;
					std::pair<uint, uint> maleFirstNames;
					std::pair<uint, uint> femaleFirstNames;
					std::pair<uint, uint> lastNames;
					std::pair<uint, uint> largeShipNames;
					uint largeShipDesignation;
					while (ini.read_value())
					{
						if (ini.is_value("affiliation"))
						{
							factionId = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("firstname_male"))
						{
							maleFirstNames.first = ini.get_value_int(0);
							maleFirstNames.second = ini.get_value_int(1);
						}
						else if (ini.is_value("firstname_female"))
						{
							femaleFirstNames.first = ini.get_value_int(0);
							femaleFirstNames.second = ini.get_value_int(1);
						}
						else if (ini.is_value("lastname"))
						{
							lastNames.first = ini.get_value_int(0);
							lastNames.second = ini.get_value_int(1);
						}
						else if (ini.is_value("large_ship_names"))
						{
							largeShipNames.first = ini.get_value_int(0);
							largeShipNames.second = ini.get_value_int(1);
						}
						else if (ini.is_value("large_ship_desig"))
						{
							largeShipDesignation = ini.get_value_int(0);
						}
					}
					if (factionId)
					{
						factionMaleFirstNames.insert({ factionId, maleFirstNames });
						factionFemaleFirstNames.insert({ factionId, femaleFirstNames });
						factionLastNames.insert({ factionId, lastNames });
						factionLargeShipNames.insert({ factionId, largeShipNames });
						factionLargeShipDesignations.insert({ factionId, largeShipDesignation });
					}
				}
			}
			ini.close();
		}
	}
}