#pragma once
#include "Costumes.h"

namespace RandomMissions
{
	std::unordered_map<uint, Costume> costumeById;

	void ReadCostumeData() {
		INI_Reader ini;
		if (!ini.open("..\\DATA\\CHARACTERS\\costumes.ini", false)) return;

		while (ini.read_header())
		{
			if (ini.is_header("Costume"))
			{
				uint costumeId = 0;
				Costume costume{};
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						costumeId = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("head"))
					{
						costume.head = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("body"))
					{
						costume.body = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("righthand"))
					{
						costume.rightHand = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("lefthand"))
					{
						costume.leftHand = CreateID(ini.get_value_string(0));
					}
				}
				if (costumeId)
				{
					costumeById.insert({ costumeId, costume });
				}
			}
		}
		ini.close();
	}
}