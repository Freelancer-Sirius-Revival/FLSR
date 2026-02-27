#include <FLHook.h>
#include "TradeMissions.h"
#include "Offers.h"
#include "Factions.h"
#include "../Mission.h"
#include "../conditions/CndTimer.h"
#include "../conditions/CndDestroyed.h"
#include "../conditions/CndLeaveMsn.h"
#include "../conditions/CndBaseEnter.h"
#include "../actions/ActLeaveMsn.h"
#include "../actions/ActTerminateMsn.h"
#include "../actions/ActSetNNObj.h"
#include "../actions/ActEtherComm.h"
#include "../actions/ActAdjRep.h"
#include "../actions/ActAdjAcct.h"
#include "../actions/ActAddCargo.h"
#include "../actions/ActRemoveCargo.h"
#include "../../Plugin.h"
#include <random>

namespace RandomMissions
{
	const uint desiredMissionCount = 3;

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
	std::unordered_map<uint, std::unordered_set<uint>> missionIdsByBaseIds;

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
						{
							commoditiesPerBase[baseId].push_back(commodities.size() - 1);
							missionIdsByBaseIds.insert({ baseId, {} });
						}
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

	static uint GenerateMissionForBase(const uint baseId)
	{
		std::vector<BaseOffer> shuffledBaseFactions(offersByBaseId[baseId]);
		std::ranges::shuffle(shuffledBaseFactions, rd);

		RandomDestination destination = {};
		uint factionId = 0;
		for (const auto& faction : shuffledBaseFactions)
		{
			destination = GetRandomCommodityAndTargetBase(baseId, faction.factionId, factionById[faction.factionId].hostileFactionIds);
			if (destination.commodityIndex >= 0 && destination.baseId)
			{
				factionId = faction.factionId;
				break;
			}
		}
		if (destination.commodityIndex < 0 || !destination.baseId)
			return 0;

		if (factionById[factionId].groupId == 0)
			return 0;

		const Commodity& commodity = commodities[destination.commodityIndex];
		const uint targetBaseId = destination.baseId;
		const uint reward = destination.jumpDistance * commodity.moneyPerJump + commodity.moneyPerJump;
		const float reputation = destination.jumpDistance * commodity.reputationPerJump + commodity.reputationPerJump;

		const uint missionId = Missions::missions.size() + 1;
		const auto& result = Missions::missions.try_emplace(missionId, "", missionId, false);
		if (!result.second)
			return 0;

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
		if (!startSystemId)
			return 0;

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
		if (!targetObjId || !targetSystemId)
			return 0;

		mission.offer.type = pub::GF::MissionType::RetrieveContraband;
		mission.offer.system = targetSystemId;
		mission.offer.group = factionById[factionId].groupId;
		mission.offer.title = 524391;
		mission.offer.description = FmtStr(524392, 0);
		mission.offer.description.append_good(commodity.id);
		mission.offer.description.append_base(targetBaseId);
		mission.offer.description.append_system(targetSystemId);
		mission.offer.description.append_rep_group(mission.offer.group);
		FmtStr ships(327681, 0);
		const std::vector<uint> shipIds(commodity.allowedShips.begin(), commodity.allowedShips.end());
		for (size_t index = 0, length = shipIds.size(); index < length; index++)
		{
			const auto shipArch = Archetype::GetShip(shipIds[index]);
			if (shipArch)
			{
				if (index < 1)
					ships.append_fmt_str(FmtStr(shipArch->iIdsName, 0));
				else
				{
					FmtStr subStr(524393, 0);
					subStr.append_string(shipArch->iIdsName);
					ships.append_fmt_str(subStr);
				}
			}
		}
		mission.offer.description.append_fmt_str(ships);
		mission.offer.reward = reward;
		mission.offer.shipArchetypeIds = commodity.allowedShips;
		mission.offer.baseIds = { baseId };
		mission.offer.reofferCondition = Missions::MissionReofferCondition::Never;
		mission.offer.reofferDelay = 0.0f;

		/* Set Objective */
		{
			const uint triggerId = CreateID("setObjective");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			{
				IObjRW* inspect;
				StarSystem* system;
				if (GetShipInspect(targetObjId, inspect, system))
				{
					Missions::ActSetNNObjPtr action(new Missions::ActSetNNObj());
					action->label = CreateID("players");
					action->systemId = targetSystemId;
					action->targetObjName = targetObjId;
					action->message = FmtStr(524394, 0);
					action->message.append_good(commodity.id);
					action->message.append_base(targetBaseId);
					action->position = inspect->get_position();
					action->bestRoute = true;
					trigger.actions.push_back(action);
				}
			}

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = CreateID("players");
				msnComms->id = CreateID("msnGoToTarget");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = factionById[factionId].missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_targetatwaypoint_01-") });
				trigger.actions.push_back(msnComms);
			}
		}

		/* Add Cargo */
		{
			const uint triggerId = CreateID("addCargo");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndTimer(Missions::ConditionParent(missionId, triggerId), 1.0f, 0.0f));

			{
				Missions::ActAddCargoPtr action(new Missions::ActAddCargo());
				action->label = CreateID("initial_player");
				action->itemId = commodity.id;
				action->count = 1;
				action->missionFlagged = true;
				trigger.actions.push_back(action);
			}
		}

		/* Initial Player Death */
		{
			const uint triggerId = CreateID("death");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndDestroyed(Missions::ConditionParent(missionId, triggerId), CreateID("initial_player"), Missions::CndDestroyed::DestroyCondition::Explode, 0, 1, false));

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = CreateID("players");
				msnComms->id = CreateID("msnLostLoot");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = factionById[factionId].missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_fail_destroyedloot_02-") });
				trigger.actions.push_back(msnComms);
			}

			{
				Missions::ActAdjRepPtr adjRep(new Missions::ActAdjRep());
				adjRep->objNameOrLabel = CreateID("players");
				adjRep->groupId = mission.offer.group;
				adjRep->change = 0.0f;
				adjRep->reason = Empathies::ReputationChangeReason::MissionFailure;
				trigger.actions.push_back(adjRep);
			}

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = CreateID("players");
				leaveMsn->leaveType = Missions::LeaveMsnType::Failure;
				leaveMsn->failureStringId = 13089;
				trigger.actions.push_back(leaveMsn);
			}

			{
				Missions::ActTerminateMsnPtr endMsn(new Missions::ActTerminateMsn());
				trigger.actions.push_back(endMsn);
			}
		}

		/* Initial Player Leaves */
		{
			const uint triggerId = CreateID("leavingMsn");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLeaveMsn(Missions::ConditionParent(missionId, triggerId), CreateID("initial_player")));

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = CreateID("players");
				msnComms->id = CreateID("msnLeftLoot");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = factionById[factionId].missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_fail_destroyedloot_02-") });
				trigger.actions.push_back(msnComms);
			}

			{
				Missions::ActRemoveCargoPtr removeCargo(new Missions::ActRemoveCargo());
				removeCargo->label = CreateID("initial_player");
				removeCargo->itemId = commodity.id;
				removeCargo->count = 1;
				trigger.actions.push_back(removeCargo);
			}

			{
				Missions::ActAdjRepPtr adjRep(new Missions::ActAdjRep());
				adjRep->objNameOrLabel = CreateID("players");
				adjRep->groupId = mission.offer.group;
				adjRep->change = 0.0f;
				adjRep->reason = Empathies::ReputationChangeReason::MissionFailure;
				trigger.actions.push_back(adjRep);
			}

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = CreateID("players");
				leaveMsn->leaveType = Missions::LeaveMsnType::Failure;
				leaveMsn->failureStringId = 13086;
				trigger.actions.push_back(leaveMsn);
			}

			{
				Missions::ActTerminateMsnPtr endMsn(new Missions::ActTerminateMsn());
				trigger.actions.push_back(endMsn);
			}
		}


		/* Land on Any Base */
		{
			const uint triggerId = CreateID("landOnAnyBase");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndBaseEnter(Missions::ConditionParent(missionId, triggerId), CreateID("initial_player"), {}));

			// Remove cargo because Freelancer does not transfer MISSION CARGO flag
			{
				Missions::ActRemoveCargoPtr removeCargo(new Missions::ActRemoveCargo());
				removeCargo->label = CreateID("initial_player");
				removeCargo->itemId = commodity.id;
				removeCargo->count = 1;
				trigger.actions.push_back(removeCargo);
			}

			// Re-add cargo to keep it visible as MISSION CARGO flagged item
			{
				Missions::ActAddCargoPtr action(new Missions::ActAddCargo());
				action->label = CreateID("initial_player");
				action->itemId = commodity.id;
				action->count = 1;
				action->missionFlagged = true;
				trigger.actions.push_back(action);
			}
		}

		/* Land on Target Base */
		{
			const uint triggerId = CreateID("landOnTargetBase");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndBaseEnter(Missions::ConditionParent(missionId, triggerId), CreateID("initial_player"), { targetBaseId }));
			// No check follows if we actually have the cargo in bay. If it would be lost before, the mission would've failed anyway.

			{
				Missions::ActRemoveCargoPtr removeCargo(new Missions::ActRemoveCargo());
				removeCargo->label = CreateID("initial_player");
				removeCargo->itemId = commodity.id;
				removeCargo->count = 1;
				trigger.actions.push_back(removeCargo);
			}

			{
				Missions::ActAdjRepPtr adjRep(new Missions::ActAdjRep());
				adjRep->objNameOrLabel = CreateID("players");
				adjRep->groupId = mission.offer.group;
				adjRep->change = reputation;
				trigger.actions.push_back(adjRep);
			}

			{
				Missions::ActAdjAcctPtr adjAcct(new Missions::ActAdjAcct());
				adjAcct->label = CreateID("players");
				adjAcct->cash = reward;
				adjAcct->splitBetweenPlayers = true;
				trigger.actions.push_back(adjAcct);
			}

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = CreateID("players");
				msnComms->id = CreateID("msnCargoDelivered");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = factionById[factionId].missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_success_returnloot_01-") });
				trigger.actions.push_back(msnComms);
			}

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = CreateID("players");
				leaveMsn->leaveType = Missions::LeaveMsnType::Success;
				trigger.actions.push_back(leaveMsn);
			}

			{
				Missions::ActTerminateMsnPtr endMsn(new Missions::ActTerminateMsn());
				endMsn->deleteMission = true;
				trigger.actions.push_back(endMsn);
			}
		}

		return mission.id;
	}

	namespace Hooks
	{
		namespace TradeMissions
		{
			void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
			{
				if (auto entry = missionIdsByBaseIds.find(baseId); entry != missionIdsByBaseIds.end())
				{
					// Remove all accepted missions from the saved missions-in-offer.
					const std::unordered_set<uint> missionIds(entry->second);
					for (const uint missionId : missionIds)
						if (Missions::missions.at(missionId).IsActive())
							entry->second.erase(missionId);


					// Add new missions to the board until it reached the desired count.
					for (size_t index = entry->second.size(); index < desiredMissionCount; index++)
					{
						const uint missionId = GenerateMissionForBase(entry->first);
						if (missionId)
							entry->second.insert(missionId);
					}
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}