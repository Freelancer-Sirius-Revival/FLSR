#pragma once
#include "Offers.h"

namespace RandomMissions
{
	std::unordered_map<uint, std::vector<BaseOffer>> offersByBaseId;
	std::unordered_map<uint, std::pair<byte, byte>> offerCountByBaseId;

	void ReadOfferData() {
		INI_Reader ini;
		if (!ini.open("..\\DATA\\MISSIONS\\mbases.ini", false)) return;

		uint baseId = 0;
		while (ini.read_header())
		{
			if (ini.is_header("MBase"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						baseId = CreateID(ini.get_value_string(0));
					}
				}
			}
			else if (ini.is_header("MVendor"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("num_offers"))
					{
						offerCountByBaseId[baseId] = { ini.get_value_int(0), ini.get_value_int(1) };
					}
				}
			}
			else if (ini.is_header("BaseFaction"))
			{
				BaseOffer offer{};
				offer.baseId = baseId;
				while (ini.read_value())
				{
					if (ini.is_value("faction"))
					{
						offer.factionId = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("mission_type"))
					{
						offer.minDifficulty = ini.get_value_float(1);
						offer.maxDifficulty = ini.get_value_float(2);
						offer.weight = ini.get_value_float(3);
					}
				}
				offersByBaseId[baseId].push_back(offer);
			}
		}
		ini.close();
	}
}