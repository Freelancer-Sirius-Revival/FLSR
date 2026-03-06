#include "RandomMissions.h"
#include "TradeMissions.h"
#include "Costumes.h"
#include "Factions.h"
#include "Meta.h"
#include "Offers.h"

namespace RandomMissions
{
	void ReadData()
	{
		ConPrint(L"Reading Random Missions... ");

		ReadCostumeData(); // Must be called before faction data
		ReadFactionData();
		ReadMetadata();
		ReadOfferData();
		ReadTradeCommoditiesData();

		ConPrint(L"Done\n");
	}

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		ConPrint(L"Initializing Random Missions... ");

		CacheDockableSolars();

		for (auto& factionEntry : factionById)
			pub::Reputation::GetReputationGroup(factionEntry.second.groupId, factionEntry.second.name.c_str());

		ConPrint(L"Done\n");
	}
}