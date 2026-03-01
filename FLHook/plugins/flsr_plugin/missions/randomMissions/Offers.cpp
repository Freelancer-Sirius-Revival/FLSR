#include "Offers.h"

namespace RandomMissions
{
	std::unordered_map<uint, uint> owningFactionByBaseId;
	std::unordered_map<uint, std::vector<BaseOffer>> offersByBaseId;

	void ReadOfferData()
	{
		INI_Reader ini;
		if (!ini.open("..\\DATA\\MISSIONS\\mbases.ini", false)) return;

		uint baseId = 0;
		uint owningFaction = 0;
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
					else if (ini.is_value("local_faction"))
					{
						owningFaction = CreateID(ini.get_value_string(0));
					}
				}
				if (baseId)
					owningFactionByBaseId[baseId] = owningFaction;
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
				// Also add factions that may not offer normal missions. This is still used for trade missions.
				offersByBaseId[baseId].push_back(offer);
			}
		}
		ini.close();
	}
}