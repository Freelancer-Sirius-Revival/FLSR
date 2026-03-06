#include <FLHook.h>
#include "TradeMissions.h"
#include "Offers.h"
#include "Factions.h"
#include "Meta.h"
#include "../Mission.h"
#include "../conditions/CndBaseEnter.h"
#include "../conditions/CndDestroyed.h"
#include "../conditions/CndJoinGroup.h"
#include "../conditions/CndLaunchComplete.h"
#include "../conditions/CndLeaveMsn.h"
#include "../conditions/CndLeaveGroup.h"
#include "../conditions/CndTimer.h"
#include "../actions/ActAddCargo.h"
#include "../actions/ActAddLabel.h"
#include "../actions/ActAdjAcct.h"
#include "../actions/ActAdjRep.h"
#include "../actions/ActEtherComm.h"
#include "../actions/ActLeaveGroup.h"
#include "../actions/ActLeaveMsn.h"
#include "../actions/ActPlayNN.h"
#include "../actions/ActRemoveCargo.h"
#include "../actions/ActSetNNObj.h"
#include "../actions/ActTerminateMsn.h"
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
	std::unordered_map<uint, std::unordered_set<uint>> missionIdsByClientId;

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
	
	std::unordered_map<uint, std::unordered_set<CSolar*>> dockablesByBaseId;

	void CacheDockableSolars()
	{
		CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
		while (solar != nullptr)
		{
			if ((solar->get_type() == ObjectType::Station || solar->get_type() == ObjectType::DockingRing) && solar->dockWithBaseId)
				dockablesByBaseId[solar->dockWithBaseId].insert(solar);
			solar = static_cast<CSolar*>(solar->FindNext());
		}
	}

	struct RandomDestination
	{
		Commodity* commodity = nullptr;
		uint startSystemId;
		uint targetSystemId;
		uint targetBaseId;
		uint targetObjId;
		Vector targetPosition;
		uint jumpDistance;
		uint offerFactionId;
	};

	#define ADDR_CONTENT_GETMISSIONPROPERTIES 0xB87D0
	typedef bool(__cdecl* _GetMissionProperties)(uint shipId, st6::vector<uint>& missionProps);

	static bool CanDock(const st6::vector<Archetype::EqObj::DockHardpointInfo>& dockInfos, const std::unordered_set<uint>& missionProperties)
	{
		for (const auto& dockInfo : dockInfos)
		{
			bool result;
			switch (dockInfo.dockType)
			{
				case Archetype::DockType::Berth:
				case Archetype::DockType::Ring:
					result = missionProperties.contains(0x87309f87); // CRC of 'can_use_berths'
					break;

				//case Archetype::DockType::MoorSmall:
					//result = missionProperties.contains(CreateID("can_use_small_moors")); // Not read from mission properties in shiparch
					//break;

				case Archetype::DockType::MoorMedium:
					result = missionProperties.contains(0xa2424943); // CRC of 'can_use_medium_moors' which is 'can_use_med_moors' in shiparch
					break;

				case Archetype::DockType::MoorLarge:
					result = missionProperties.contains(0xa6cc80c2); // CRC of 'can_use_large_moors'
					break;

				case Archetype::DockType::Jump:
				case Archetype::DockType::Airlock:
					result = true;
					break;

				default:
					result = false;
			}
			if (result)
				return true;
		}
		return false;
	}

	static RandomDestination GetRandomDestination(const uint clientId, const uint startBaseId, const uint shipArchetypeId)
	{
		RandomDestination result = {};

		int clientRep = 0;
		pub::Player::GetRep(clientId, clientRep);
		if (!clientRep)
			return result;

		if (!commoditiesPerBase.contains(startBaseId) || commoditiesPerBase.at(startBaseId).empty())
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

		_GetMissionProperties GetMissionProperties = (_GetMissionProperties)CONTENT_ADDR(ADDR_CONTENT_GETMISSIONPROPERTIES);
		st6::vector<uint> missionPropsResult;
		if (!GetMissionProperties(shipArchetypeId, missionPropsResult) || missionPropsResult.empty())
			return result;
		const std::unordered_set<uint> missionProps(missionPropsResult.begin(), missionPropsResult.end());

		std::vector<size_t> shuffledCommodityIndices = commoditiesPerBase[startBaseId];
		std::ranges::shuffle(shuffledCommodityIndices, rd);
		std::vector<Commodity*> shuffledCommodities;
		for (const auto& commodityIndex : shuffledCommodityIndices)
			shuffledCommodities.push_back(&commodities.at(commodityIndex));

		std::vector<BaseOffer> shuffledBaseFactions(offersByBaseId.at(startBaseId));
		std::ranges::shuffle(shuffledBaseFactions, rd);
		for (const auto& faction : shuffledBaseFactions)
		{
			float feelings = -1.0f;
			pub::Reputation::GetGroupFeelingsTowards(clientRep, factionById.at(faction.factionId).groupId, feelings);
			if (feelings <= RandomMissions::minRequiredReputationForMissions)
				continue;

			const auto& hostileFactionIds = factionById.at(faction.factionId).hostileFactionIds;
			for (const auto& commodity : shuffledCommodities)
			{
				if (!commodity->allowedShips.empty() && !commodity->allowedShips.contains(shipArchetypeId))
					continue;

				std::vector<uint> shuffledBaseIds(commodity->receiverBases.begin(), commodity->receiverBases.end());
				std::ranges::shuffle(shuffledBaseIds, rd);
				for (const uint targetBaseId : shuffledBaseIds)
				{
					if (startBaseId == targetBaseId)
						continue;

					const auto& dockablesEntry = dockablesByBaseId.find(targetBaseId);
					if (dockablesEntry == dockablesByBaseId.end())
						continue;

					CSolar* dockable = nullptr;
					for (const auto& dockableObj : dockablesEntry->second)
					{
						if (CanDock(static_cast<Archetype::Solar*>(dockableObj->archetype)->dockInfo, missionProps))
						{
							dockable = dockableObj;
							break;
						}
					}
					if (!dockable)
						continue;
					
					if (hostileFactionIds.contains(owningFactionByBaseId[targetBaseId]))
						continue;

					if (shortestSystemPathToTargets.at(startSystemId).at(dockable->system).size() - 1 < commodity->minJumpDistance)
						continue;

					uint affiliationId;
					pub::Reputation::GetAffiliation(dockable->id, affiliationId);
					if (affiliationId)
					{
						float feelings = -1.0f;
						pub::Reputation::GetGroupFeelingsTowards(clientRep, affiliationId, feelings);
						if (feelings <= hostileDockingThreshold)
							continue;
					}

					result.commodity = commodity;
					result.startSystemId = startSystemId;
					result.targetSystemId = dockable->system;
					result.targetObjId = dockable->id;
					result.targetBaseId = targetBaseId;
					result.targetPosition = dockable->get_position();
					result.jumpDistance = shortestSystemPathToTargets.at(startSystemId).at(dockable->system).size();
					result.offerFactionId = faction.factionId;
					return result;
				}
			}
		}
		return result;
	}

	const uint InitialPlayer = CreateID("initial_player");
	const uint Players = CreateID("players");

	static uint GenerateMissionForClient(const uint clientId)
	{
		uint startBaseId = 0;
		pub::Player::GetBase(clientId, startBaseId);
		if (!startBaseId)
			return 0;

		uint shipArchetypeId = 0;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		if (!shipArchetypeId)
			return 0;

		std::vector<BaseOffer> shuffledBaseFactions(offersByBaseId.at(startBaseId));
		std::ranges::shuffle(shuffledBaseFactions, rd);

		const RandomDestination& destination = GetRandomDestination(clientId, startBaseId, shipArchetypeId);
		if (!destination.commodity)
			return 0;

		const uint missionId = Missions::missions.size() + 1;
		const auto& result = Missions::missions.try_emplace(missionId, "", missionId, false, false);
		if (!result.second)
			return 0;

		const Commodity& commodity = *destination.commodity;
		const auto& offerFaction = factionById.at(destination.offerFactionId);
		const float reputation = destination.jumpDistance * commodity.reputationPerJump + commodity.reputationPerJump;

		Missions::Mission& mission = result.first->second;
		mission.offer.type = pub::GF::MissionType::RetrieveContraband;
		mission.offer.system = destination.targetSystemId;
		mission.offer.group = offerFaction.groupId;
		mission.offer.title = 524390;
		mission.offer.description = FmtStr(524391, 0);
		mission.offer.description.append_good(commodity.id);
		mission.offer.description.append_base(destination.targetBaseId);
		mission.offer.description.append_system(destination.targetSystemId);
		mission.offer.description.append_rep_group(mission.offer.group);
		mission.offer.description.append_int(reputation * 100.0f);
		mission.offer.reward = destination.jumpDistance * commodity.moneyPerJump + commodity.moneyPerJump;
		mission.offer.shipArchetypeIds = commodity.allowedShips;
		mission.offer.baseIds = {};
		mission.offer.reofferCondition = Missions::MissionReofferCondition::Never;
		mission.offer.reofferDelay = 0.0f;

		/* Set Objective */
		{
			const uint triggerId = CreateID("setObjective");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			{
				Missions::ActSetNNObjPtr action(new Missions::ActSetNNObj());
				action->label = Players;
				action->systemId = destination.targetSystemId;
				action->targetObjName = destination.targetObjId;
				action->message = FmtStr(524392, 0);
				action->message.append_good(commodity.id);
				action->message.append_base(destination.targetBaseId);
				action->position = destination.targetPosition;
				action->bestRoute = true;
				trigger.actions.push_back(action);
			}

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = Players;
				msnComms->id = CreateID("msnGoToTarget");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = offerFaction.missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_targetatwaypoint_01-") });
				trigger.actions.push_back(msnComms);
			}

			{
				Missions::ActPlayNNPtr msnComms(new Missions::ActPlayNN());
				msnComms->label = InitialPlayer;
				msnComms->soundIds = std::vector<uint>({ CreateID("cmsn_accepted") });
				trigger.actions.push_back(msnComms);
			}
		}

		/* Joining the Group of the Mission Players */
		{
			const uint triggerId = CreateID("joinGroup");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndJoinGroup(Missions::ConditionParent(missionId, triggerId), 0));

			{
				Missions::ActAddLabelPtr addLabel(new Missions::ActAddLabel());
				addLabel->objNameOrLabel = Missions::Activator;
				addLabel->label = Players;
				trigger.actions.push_back(addLabel);
			}

			{
				Missions::ActSetNNObjPtr action(new Missions::ActSetNNObj());
				action->label = Missions::Activator;
				action->systemId = destination.targetSystemId;
				action->targetObjName = destination.targetObjId;
				action->message = FmtStr(524392, 0);
				action->message.append_good(commodity.id);
				action->message.append_base(destination.targetBaseId);
				action->position = destination.targetPosition;
				action->bestRoute = true;
				trigger.actions.push_back(action);
			}

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = Missions::Activator;
				msnComms->id = CreateID("msnGoToTarget");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = offerFaction.missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_targetatwaypoint_01-") });
				trigger.actions.push_back(msnComms);
			}
		}

		/* Leaving the Group of the Mission Players */
		{
			const uint triggerId = CreateID("leaveGroup");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLeaveGroup(Missions::ConditionParent(missionId, triggerId), 0));

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = Missions::Activator;
				leaveMsn->leaveType = Missions::LeaveMsnType::Failure;
				leaveMsn->failureStringId = 13086;
				trigger.actions.push_back(leaveMsn);
			}
		}

		/* Launch from Any Base */
		{
			const uint triggerId = CreateID("launchFromAnyBase");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLaunchComplete(Missions::ConditionParent(missionId, triggerId), Players, {}));

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = Missions::Activator;
				msnComms->id = CreateID("msnGoToTarget");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = offerFaction.missionCommission;
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
				action->label = InitialPlayer;
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

			trigger.condition = Missions::ConditionPtr(new Missions::CndDestroyed(Missions::ConditionParent(missionId, triggerId), InitialPlayer, Missions::CndDestroyed::DestroyCondition::Explode, 0, 1, false));

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = Players;
				msnComms->id = CreateID("msnLostLoot");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = offerFaction.missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_fail_destroyedloot_02-") });
				trigger.actions.push_back(msnComms);
			}

			{
				Missions::ActAdjRepPtr adjRep(new Missions::ActAdjRep());
				adjRep->objNameOrLabel = Players;
				adjRep->groupId = mission.offer.group;
				adjRep->change = 0.0f;
				adjRep->reason = Empathies::ReputationChangeReason::MissionFailure;
				trigger.actions.push_back(adjRep);
			}

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = Players;
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
			const uint triggerId = CreateID("leavingMsnInitialPlayer");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLeaveMsn(Missions::ConditionParent(missionId, triggerId), InitialPlayer));

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = Players;
				msnComms->id = CreateID("msnLeftLoot");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = offerFaction.missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_fail_destroyedloot_02-") });
				trigger.actions.push_back(msnComms);
			}

			{
				Missions::ActRemoveCargoPtr removeCargo(new Missions::ActRemoveCargo());
				removeCargo->label = InitialPlayer;
				removeCargo->itemId = commodity.id;
				removeCargo->count = 1;
				trigger.actions.push_back(removeCargo);
			}

			{
				Missions::ActAdjRepPtr adjRep(new Missions::ActAdjRep());
				adjRep->objNameOrLabel = Players;
				adjRep->groupId = mission.offer.group;
				adjRep->change = 0.0f;
				adjRep->reason = Empathies::ReputationChangeReason::MissionFailure;
				trigger.actions.push_back(adjRep);
			}

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = Players;
				leaveMsn->leaveType = Missions::LeaveMsnType::Failure;
				leaveMsn->failureStringId = 13086;
				trigger.actions.push_back(leaveMsn);
			}

			{
				Missions::ActTerminateMsnPtr endMsn(new Missions::ActTerminateMsn());
				trigger.actions.push_back(endMsn);
			}
		}

		/* Any Player Leaves */
		{
			const uint triggerId = CreateID("leavingMsnPlayers");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLeaveMsn(Missions::ConditionParent(missionId, triggerId), Players));

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = Missions::Activator;
				leaveMsn->leaveType = Missions::LeaveMsnType::Failure;
				leaveMsn->failureStringId = 13086;
				trigger.actions.push_back(leaveMsn);
			}

			{
				Missions::ActLeaveGroupPtr leaveGroup(new Missions::ActLeaveGroup());
				leaveGroup->label = Missions::Activator;
				trigger.actions.push_back(leaveGroup);
			}
		}

		/* Land on Any Base */
		{
			const uint triggerId = CreateID("landOnAnyBase");
			mission.triggers.try_emplace(triggerId, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndBaseEnter(Missions::ConditionParent(missionId, triggerId), InitialPlayer, {}));

			// Remove cargo because Freelancer does not transfer MISSION CARGO flag
			{
				Missions::ActRemoveCargoPtr removeCargo(new Missions::ActRemoveCargo());
				removeCargo->label = InitialPlayer;
				removeCargo->itemId = commodity.id;
				removeCargo->count = 1;
				trigger.actions.push_back(removeCargo);
			}

			// Re-add cargo to keep it visible as MISSION CARGO flagged item
			{
				Missions::ActAddCargoPtr action(new Missions::ActAddCargo());
				action->label = InitialPlayer;
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

			trigger.condition = Missions::ConditionPtr(new Missions::CndBaseEnter(Missions::ConditionParent(missionId, triggerId), InitialPlayer, { destination.targetBaseId }));
			// No check follows if we actually have the cargo in bay. If it would be lost before, the mission would've failed anyway.

			{
				Missions::ActRemoveCargoPtr removeCargo(new Missions::ActRemoveCargo());
				removeCargo->label = InitialPlayer;
				removeCargo->itemId = commodity.id;
				removeCargo->count = 1;
				trigger.actions.push_back(removeCargo);
			}

			{
				Missions::ActAdjRepPtr adjRep(new Missions::ActAdjRep());
				adjRep->objNameOrLabel = Players;
				adjRep->groupId = mission.offer.group;
				adjRep->change = reputation;
				trigger.actions.push_back(adjRep);
			}

			{
				Missions::ActAdjAcctPtr adjAcct(new Missions::ActAdjAcct());
				adjAcct->label = Players;
				adjAcct->cash = mission.offer.reward;
				adjAcct->splitBetweenPlayers = true;
				trigger.actions.push_back(adjAcct);
			}

			{
				Missions::ActEtherCommPtr msnComms(new Missions::ActEtherComm());
				msnComms->receiverObjNameOrLabel = Players;
				msnComms->id = CreateID("msnCargoDelivered");
				msnComms->senderVoiceId = CreateID("mc_leg_m01");
				msnComms->senderIdsName = 13015;
				msnComms->costume = offerFaction.missionCommission;
				msnComms->lines = std::vector<uint>({ CreateID("rmb_success_returnloot_01-") });
				trigger.actions.push_back(msnComms);
			}

			{
				Missions::ActLeaveMsnPtr leaveMsn(new Missions::ActLeaveMsn());
				leaveMsn->label = Players;
				leaveMsn->leaveType = Missions::LeaveMsnType::Success;
				trigger.actions.push_back(leaveMsn);
			}

			{
				Missions::ActTerminateMsnPtr endMsn(new Missions::ActTerminateMsn());
				trigger.actions.push_back(endMsn);
			}
		}

		mission.TryAddToJobBoard(clientId);

		return mission.id;
	}

	namespace Hooks
	{
		namespace TradeMissions
		{
			void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
			{
				// Add new missions to the client's mission board. By vanilla FL behaviour this also happens even if the player is already in a mission.
				for (size_t index = 0; index < desiredMissionCount; index++)
				{
					const uint missionId = GenerateMissionForClient(clientId);
					if (missionId)
						missionIdsByClientId[clientId].insert(missionId);
				}

				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
			{
				// Delete all generated missions the client hasn't activated.
				for (const uint missionId : missionIdsByClientId[clientId])
				{
					const auto& missionEntry = Missions::missions.find(missionId);
					if (missionEntry == Missions::missions.end())
						continue;

					if (missionEntry->second.IsActive())
						continue;

					Missions::missions.erase(missionEntry);
				}

				// Clear the mission board for the client. Any active missions will run until they are properly finished.
				missionIdsByClientId.erase(clientId);

				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}