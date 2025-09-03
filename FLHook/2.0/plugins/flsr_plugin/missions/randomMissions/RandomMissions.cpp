#pragma once
#include "RandomMissions.h"
#include "Costumes.h"
#include "Factions.h"
#include "Meta.h"
#include "NpcShips.h"
#include "NpcWaves.h"
#include "Offers.h"
#include "Rewards.h"
#include "Vignette.h"

namespace RandomMissions
{
	void ReadData() {
		ReadCostumeData(); // Must be called before faction data
		ReadFactionData();
		ReadMetadata();
		ReadNpcShipData();
		ReadNpcWaveData();
		ReadOfferData();
		ReadRewardData();
		ReadVignetteData();
	}
}