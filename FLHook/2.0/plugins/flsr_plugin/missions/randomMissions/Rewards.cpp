#pragma once
#include "Rewards.h"

namespace RandomMissions
{
	std::unordered_map<float, uint> moneyByDifficulty;

	void ReadRewardData() {
		INI_Reader ini;
		if (ini.open("..\\DATA\\RANDOMMISSIONS\\Diff2Money.ini", false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Diff2Money"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("Diff2Money"))
						{
							moneyByDifficulty.insert(
								{ ini.get_value_float(0), ini.get_value_int(1) }
							);
						}
					}
				}
			}
			ini.close();
		}
	}
}