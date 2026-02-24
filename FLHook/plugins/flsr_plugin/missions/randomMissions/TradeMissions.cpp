#include "TradeMissions.h"
#include <random>
#include "Offers.h"
#include "Factions.h"
#include "../Mission.h"
#include "../actions/ActActTrig.h"
#include "../actions/ActSetNNObj.h"
#include "../actions/ActStartDialog.h"
#include "../actions/ActAdjRep.h"
#include "../actions/ActAdjAcct.h"
#include "../actions/ActAddCargo.h"
#include "../actions/ActRemoveCargo.h"

namespace RandomMissions
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	struct Commodity
	{
		uint id = 0;
		std::unordered_set<uint> offerBases;
		std::unordered_set<uint> receiverBases;
		std::unordered_set<uint> offerFactions;
		std::unordered_set<uint> allowedShips;
		uint moneyPerJump = 0;
		float reputationPerJump = 0.0f;
		uint minJumpDistance = 0;
	};

	std::vector<Commodity> commodities;
	std::unordered_map<uint, std::vector<size_t>> commoditiesPerBase;
	std::unordered_map<uint, std::unordered_map<uint, std::vector<uint>>> shortestSystemPathToTargets;

	void ReadTradeCommoditiesData()
	{
		INI_Reader ini;

		if (ini.open("..\\DATA\\RANDOMMISSIONS\\tradeCommodities.ini", false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("TradeRoutes"))
				{
					Commodity commodity;
					while (ini.read_value())
					{
						if (ini.is_value("commodity"))
						{
							commodity.id = CreateID(ini.get_value_string(0));
						}
						else if (ini.is_value("offer_base"))
						{
							commodity.offerBases.insert(CreateID(ini.get_value_string(0)));
						}
						else if (ini.is_value("receiver_base"))
						{
							commodity.receiverBases.insert(CreateID(ini.get_value_string(0)));
						}
						else if (ini.is_value("faction"))
						{
							commodity.offerFactions.insert(CreateID(ini.get_value_string(0)));
						}
						else if (ini.is_value("allowed_ship"))
						{
							commodity.allowedShips.insert(CreateID(ini.get_value_string(0)));
						}
						else if (ini.is_value("credits_per_jump"))
						{
							commodity.moneyPerJump = ini.get_value_int(0);
						}
						else if (ini.is_value("reputation_per_jump"))
						{
							commodity.reputationPerJump = ini.get_value_float(0);
						}
						else if (ini.is_value("min_jumps"))
						{
							commodity.minJumpDistance = ini.get_value_int(0);
						}
					}
					if (commodity.id && !commodity.offerBases.empty() && !commodity.receiverBases.empty())
					{
						commodities.push_back(commodity);
						for (const auto& baseId : commodity.offerBases)
							commoditiesPerBase[baseId].push_back(commodities.size() - 1);
					}
				}
			}
			ini.close();
		}

		std::string systemsShortestPathFilePath = "";
		if (ini.open("freelancer.ini", false))
		{
			while (ini.read_header() && systemsShortestPathFilePath.empty())
			{
				if (ini.is_header("Freelancer"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("data path"))
						{
							systemsShortestPathFilePath = ini.get_value_string(0);
							systemsShortestPathFilePath += "\\UNIVERSE\\systems_shortest_path.ini";
							break;
						}
					}
				}
			}
			ini.close();
		}

		if (!systemsShortestPathFilePath.empty() && ini.open(systemsShortestPathFilePath.c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("SystemConnections"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("Path") && ini.get_num_parameters() > 2)
						{
							const uint startSystemId = CreateID(ini.get_value_string(0));
							const uint targetSystemId = CreateID(ini.get_value_string(1));
							for (size_t index = 2; index < ini.get_num_parameters(); index++)
								shortestSystemPathToTargets[startSystemId][targetSystemId].push_back(CreateID(ini.get_value_string(index)));
						}
					}
				}
			}
			ini.close();
		}

	}

	static std::pair<uint, uint> GetRandomTargetBaseIdAndDistance(const uint startSystemId, const uint startBaseId, const std::unordered_set<uint>& bases, const uint minJumpDistance, const std::unordered_set<uint>& hostileFactionIds)
	{
		if (bases.empty())
			return { 0, 0 };

		std::vector<uint> shuffledBaseIds(bases.begin(), bases.end());
		std::ranges::shuffle(shuffledBaseIds, rd);
		for (uint targetBaseId : shuffledBaseIds)
		{
			if (startBaseId == targetBaseId)
				continue;

			for (const auto& base : lstBases)
			{
				if (base.iBaseID == targetBaseId)
				{
					if (!hostileFactionIds.contains(owningFactionByBaseId[base.iBaseID]) &&
						((startSystemId == base.iSystemID && minJumpDistance == 0) ||
						(shortestSystemPathToTargets[startSystemId][base.iSystemID].size() > minJumpDistance))
					   )
						return { base.iBaseID, shortestSystemPathToTargets[startSystemId][base.iSystemID].size() };
					break;
				}
			}
		}
		return { 0, 0 };
	}

	struct RandomDestination
	{
		int commodityIndex = -1;
		uint baseId = 0;
		uint jumpDistance = 0;
	};

	static RandomDestination GetRandomCommodityAndTargetBase(const uint startBaseId, const uint offeringFaction, const std::unordered_set<uint>& hostileFactionIds)
	{
		RandomDestination result = {};
		if (!commoditiesPerBase.contains(startBaseId) || commoditiesPerBase[startBaseId].empty())
			return result;

		uint startSystemId = 0;
		for (const auto& base : lstBases)
		{
			if (base.iBaseID == startBaseId)
			{
				startSystemId = base.iSystemID;
				break;
			}
		}
		if (!startSystemId)
			return result;

		std::vector<size_t> shuffledCommodityIndices = commoditiesPerBase[startBaseId];
		std::ranges::shuffle(shuffledCommodityIndices, rd);
		for (size_t commodityIndex : shuffledCommodityIndices)
		{
			const Commodity& commodity = commodities[commodityIndex];
			if (commodity.receiverBases.empty() || !commodity.offerFactions.contains(offeringFaction))
				continue;

			std::vector<uint> shuffledBaseIds(commodity.receiverBases.begin(), commodity.receiverBases.end());
			std::ranges::shuffle(shuffledBaseIds, rd);
			const auto& base = GetRandomTargetBaseIdAndDistance(startSystemId, startBaseId, commodity.receiverBases, commodity.minJumpDistance, hostileFactionIds);
			if (base.first)
			{
				result.commodityIndex = commodityIndex;
				result.baseId = base.first;
				result.jumpDistance = base.second;
				break;
			}
		}
		return result;
	}

	int GenerateMissionForBase(const uint baseId)
	{
		std::vector<BaseOffer> shuffledBaseFactions(offersByBaseId[baseId]);
		std::ranges::shuffle(shuffledBaseFactions, rd);

		RandomDestination destination = {};
		for (const auto& faction : shuffledBaseFactions)
		{
			destination = GetRandomCommodityAndTargetBase(baseId, faction.factionId, factionById[faction.factionId].hostileFactionIds);
			if (destination.commodityIndex >= 0 && destination.baseId)
				break;
		}
		if (destination.commodityIndex < 0 || !destination.baseId)
			return -1;

		const Commodity& commodity = commodities[destination.commodityIndex];
		const uint targetBaseId = destination.baseId;
		const uint reward = destination.jumpDistance * commodity.moneyPerJump + commodity.moneyPerJump;
		const float reputation = destination.jumpDistance * commodity.reputationPerJump + commodity.reputationPerJump;

		const uint missionId = Missions::missions.size() + 1;
		const auto& result = Missions::missions.try_emplace(missionId, "", missionId, false);
		if (!result.second)
			return -1;

		Missions::Mission& mission = result.first->second;

		uint startSystemId = 0;
		for (const auto& base : lstBases)
		{
			if (base.iBaseID == baseId)
			{
				startSystemId = base.iSystemID;
				break;
			}
		}

		mission.offer.type = pub::GF::MissionType::RetrieveContraband;
		mission.offer.system = startSystemId;
		//uint groupName = 0;
		//pub::Reputation::GetReputationGroup(mission.offer.group, groupName);
		mission.offer.title = 327757;
		mission.offer.description = 327757;
		mission.offer.reward = reward;
		mission.offer.shipArchetypeIds = commodity.allowedShips;
		mission.offer.baseIds = { baseId };
		mission.offer.reofferCondition = Missions::MissionReofferCondition::Never;
		mission.offer.reofferDelay = 0.0f;

		uint triggerId = CreateID("init");
		mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
		Missions::Trigger& trigger = mission.triggers.at(triggerId);

		uint targetObjId = 0;
		uint targetSystemId = 0;
		for (const auto& base : lstBases)
		{
			if (base.iBaseID == destination.baseId)
			{
				targetSystemId = base.iSystemID;
				targetObjId = base.iObjectID;
				break;
			}
		}
		IObjRW* inspect;
		StarSystem* system;
		GetShipInspect(targetObjId, inspect, system);
		Missions::ActSetNNObjPtr action(new Missions::ActSetNNObj());
		action->label = CreateID("players");
		action->systemId = targetSystemId;
		action->targetObjName = targetObjId;
		action->message = 327757;
		action->position = inspect->get_position();
		action->bestRoute = true;
		trigger.actions.push_back(action);
	}
}