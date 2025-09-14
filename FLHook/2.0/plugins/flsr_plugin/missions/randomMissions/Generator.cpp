#include "Generator.h"
#include "Offers.h"
#include "Factions.h"
#include "Vignette.h"
#include "../Mission.h"

namespace RandomMissions
{
	void GenerateMission(uint clientId, uint baseId) {
		auto offers = offersByBaseId.at(baseId);
		auto hostileFactions = factionById.at(offers[0].factionId).hostileFactionIds;
		uint systemId;
		pub::Player::GetSystem(clientId, systemId);
		auto vignettes = vignettesBySystemId.at(systemId);
		for (const auto& vignette : vignettes)
		{
			if (vignette.factionIds.contains(*hostileFactions.begin()))
			{
				Missions::Mission& a = (*Missions::missions.try_emplace(1234, "Test Mission", 1234, false).first).second;
				a.offer.bases.push_back(baseId);
				a.offer.group = offers[0].factionId;
				a.offer.reward = 10000000;
				a.offer.system = systemId;
				a.offer.text = 460584;
			}
		}
	}
}