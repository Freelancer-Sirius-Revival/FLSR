#include "LootProps.h"
#include <FLHook.h>
#include <random>

namespace LootProps
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	struct DropInfo
	{
		float chance = 0.0f;
		ushort min = 0;
		ushort max1 = 0;
		ushort max2 = 0;
	};

	std::unordered_map<uint, DropInfo> dropInfoByArchetypeId;

	unsigned short CalculateDropCount(const unsigned int archetypeId, const int count)
	{
		const auto& foundEntry = dropInfoByArchetypeId.find(archetypeId);
		if (foundEntry == dropInfoByArchetypeId.end())
			return 0;
		const DropInfo& info = foundEntry->second;

		/*
			drop_properties = chance, min_worth, worth_mult, min, max1, max2

			chance is divided by 100
			worth is your current worth
			rand is a random number between 0 and 1
			target_count is how many items the droppee has of this loot
			count is how many items to drop

			count = (worth - min_worth) / worth_mult
			if (count >= target_count)
			  count = target_count
			if (count + min > max1)
			  count = max1
			else
			  count += min
			if (count >= target_count)
			  count = target_count
			prob = chance * count
			count = floor( prob )
			prob -= count
			if (rand < prob)
			  ++count
			if (count >= max2)
			  count = max2
		*/

		int resultCount = std::min<int>(info.min, info.max1);
		resultCount = std::min<int>(resultCount, count);
		float probability = info.chance * resultCount;
		resultCount = static_cast<int>(std::floor(probability));
		probability -= resultCount;
		if (std::uniform_real_distribution<float>(0, 1)(gen) < probability)
			++resultCount;
		resultCount = std::min<int>(resultCount, info.max2);
		return std::max<int>(resultCount, 0);
	}

	void ReadFiles()
	{
		std::string dataPath = "..\\data";
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

		if (ini.open((dataPath + "\\missions\\lootprops.ini").c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("mLootProps"))
				{
					DropInfo dropInfo;
					uint id = 0;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
						{
							id = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("drop_properties"))
						{
							dropInfo.chance = std::abs(ini.get_value_float(0) / 100.0f);
							dropInfo.min = std::max<ushort>(ini.get_value_int(3), 0);
							dropInfo.max1 = std::max<ushort>(ini.get_value_int(4), 0);
							dropInfo.max2 = std::max<ushort>(ini.get_value_int(5), 0);
						}
					}
					if (id)
						dropInfoByArchetypeId.insert({ id, dropInfo });
				}
			}
			ini.close();
		}
	}
}