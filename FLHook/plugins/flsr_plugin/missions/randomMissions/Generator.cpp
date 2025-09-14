#include "Generator.h"
#include "Offers.h"
#include "Factions.h"
#include "Vignette.h"
#include "../Mission.h"
#include "../MissionBoard.h"

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
				Missions::Mission& mission = (*Missions::missions.try_emplace(1234, "Test Mission", 1234, false).first).second;
				mission.offer.bases.push_back(baseId);
				mission.offer.group = offers[0].factionId;
				mission.offer.reward = 10000000;
				mission.offer.system = systemId;
				mission.offer.description = 460584;
				mission.offer.title = 458806;
				mission.offer.type = pub::GF::MissionType::DestroyShips;

				MissionBoard::MissionOffer offer
				{
					.type = mission.offer.type,
					.system = mission.offer.system,
					.group = mission.offer.group,
					.title = mission.offer.title,
					.description = mission.offer.description,
					.reward = mission.offer.reward
				};

				mission.offerId = MissionBoard::AddMissionOffer(offer, mission.offer.bases);
			}
		}
	}
}