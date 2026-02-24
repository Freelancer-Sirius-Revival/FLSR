#include "RandomMissions.h"
#include "TradeMissions.h"
#include "Costumes.h"
#include "Factions.h"
#include "Offers.h"

namespace RandomMissions
{
	void ReadData()
	{
		ReadCostumeData(); // Must be called before faction data
		ReadFactionData();
		ReadOfferData();
		ReadTradeCommoditiesData();
	}

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		GenerateMissionForBase(CreateID("li01_01_base"));
	}
}