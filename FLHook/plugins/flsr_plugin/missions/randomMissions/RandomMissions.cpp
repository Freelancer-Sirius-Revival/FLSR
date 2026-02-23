#include "RandomMissions.h"
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
	}
}