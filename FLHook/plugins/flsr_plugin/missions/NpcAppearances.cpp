#include "NpcAppearances.h"
#include <random>

namespace NpcAppearances
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	enum class Gender
	{
		None,
		Male,
		Female
	};

	static std::unordered_map<uint, Gender> voiceGenders;
	static std::unordered_map<uint, std::vector<uint>> factionVoices;
	static std::unordered_map<uint, std::pair<uint, uint>> factionMaleFirstNames;
	static std::unordered_map<uint, std::pair<uint, uint>> factionFemaleFirstNames;
	static std::unordered_map<uint, std::pair<uint, uint>> factionLastNames;
	static std::unordered_map<uint, std::pair<uint, uint>> factionLargeShipNames;
	static std::unordered_map<uint, uint> factionLargeShipDesignations;
	static std::unordered_map<uint, std::vector<Costume>> factionMaleCostumes;
	static std::unordered_map<uint, std::vector<Costume>> factionFemaleCostumes;

	uint GetRandomVoiceId(const uint factionId)
	{
		uint result = 0;
		if (const auto& voiceEntry = factionVoices.find(factionId); voiceEntry != factionVoices.end() && voiceEntry->second.size() > 0)
		{
			const auto index = std::uniform_int_distribution(size_t(0), voiceEntry->second.size() - 1)(gen);
			result = voiceEntry->second[index];
		}
		return result;
	}

	std::pair<uint, uint> GetRandomName(const uint factionId, const uint voiceId)
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

	std::pair<uint, uint> GetRandomLargeShipName(const uint factionId)
	{
		std::pair<uint, uint> result = { 0, 0 };

		if (const auto& nameEntry = factionLargeShipDesignations.find(factionId); nameEntry != factionLargeShipDesignations.end())
			result.first = nameEntry->second;

		if (const auto& nameEntry = factionLargeShipNames.find(factionId); nameEntry != factionLargeShipNames.end())
			result.second = std::uniform_int_distribution(nameEntry->second.first, nameEntry->second.second)(gen);

		return result;
	}

	Costume GetRandomCostume(const uint factionId, const uint voiceId)
	{
		Costume result;
		const auto& genderEntry = voiceGenders.find(voiceId);
		if (genderEntry == voiceGenders.end())
			return result;

		const auto& factionCostumes = genderEntry->second == Gender::Male ? factionMaleCostumes : factionFemaleCostumes;
		if (const auto& costumesEntry = factionCostumes.find(factionId); costumesEntry != factionCostumes.end() && costumesEntry->second.size() > 0)
		{
			const auto index = std::uniform_int_distribution(size_t(0), costumesEntry->second.size() - 1)(gen);
			result = costumesEntry->second[index];
		}

		return result;
	}

	void ReadFiles()
	{
		std::string dataPath = "..\\data";
		std::vector<std::string> voicePaths;
		std::vector<std::string> bodyPartsPaths;
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

						if (ini.is_value("bodyparts"))
							bodyPartsPaths.push_back(ini.get_value_string(0));
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

		std::unordered_map<uint, Gender> headGenders;
		for (const auto& filePath : bodyPartsPaths)
		{
			if (ini.open((dataPath + "\\" + filePath).c_str(), false))
			{
				Gender gender = Gender::None;
				while (ini.read_header())
				{
					if (ini.is_header("Skeleton"))
					{
						while (ini.read_value())
						{
							if (ini.is_value("sex"))
							{
								const std::string value = ToLower(ini.get_value_string(0));
								if (value == "male")
									gender = Gender::Male;
								else if (value == "female")
									gender = Gender::Female;
								else
									gender = Gender::None;
							}
						}
					}
					else if (ini.is_header("Head"))
					{
						while (ini.read_value())
						{
							if (gender != Gender::None && ini.is_value("nickname"))
								headGenders.insert({ CreateID(ini.get_value_string(0)), gender });
						}
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
					std::vector<uint> voiceIds;
					std::pair<uint, uint> maleFirstNames;
					std::pair<uint, uint> femaleFirstNames;
					std::pair<uint, uint> lastNames;
					std::pair<uint, uint> largeShipNames;
					uint largeShipDesignation;
					std::vector<Costume> maleCostumes;
					std::vector<Costume> femaleCostumes;
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
						else if (ini.is_value("voice"))
						{
							voiceIds.push_back(CreateID(ini.get_value_string(0)));
						}
						else if (ini.is_value("space_costume"))
						{
							Costume costume;

							std::string value = ini.get_value_string(0);
							if (!value.empty())
								costume.head = CreateID(value.c_str());

							value = ini.get_value_string(1);
							if (!value.empty())
								costume.body = CreateID(value.c_str());

							for (int index = 2, length = ini.get_num_parameters(); index < length; index++)
							{
								value = ini.get_value_string(index);
								if (!value.empty())
								{
									costume.accessory[costume.accessories] = CreateID(value.c_str());
									costume.accessories++;
								}
							}

							const auto& genderEntry = headGenders.find(costume.head);
							if (genderEntry != headGenders.end())
							{
								if (genderEntry->second == Gender::Male)
									maleCostumes.push_back(costume);
								else if (genderEntry->second == Gender::Female)
									femaleCostumes.push_back(costume);
							}
						}
					}
					if (factionId)
					{
						factionVoices.insert({ factionId, voiceIds });
						factionMaleFirstNames.insert({ factionId, maleFirstNames });
						factionFemaleFirstNames.insert({ factionId, femaleFirstNames });
						factionLastNames.insert({ factionId, lastNames });
						factionLargeShipNames.insert({ factionId, largeShipNames });
						factionLargeShipDesignations.insert({ factionId, largeShipDesignation });
						factionMaleCostumes.insert({ factionId, maleCostumes });
						factionFemaleCostumes.insert({ factionId, femaleCostumes });
					}
				}
			}
			ini.close();
		}
	}
}