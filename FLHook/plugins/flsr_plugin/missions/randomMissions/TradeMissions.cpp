#include <FLHook.h>
#include "TradeMissions.h"
#include "Offers.h"
#include "Factions.h"
#include "Meta.h"
#include "../Mission.h"
#include "../conditions/CndInSpace.h"
#include "../conditions/CndBaseEnter.h"
#include "../conditions/CndBaseExit.h"
#include "../conditions/CndDestroyed.h"
#include "../conditions/CndJoinGroup.h"
#include "../conditions/CndLaunchComplete.h"
#include "../conditions/CndLeaveMsn.h"
#include "../conditions/CndLeaveGroup.h"
#include "../conditions/CndTimer.h"
#include "../actions/ActActTrig.h"
#include "../actions/ActAddCargo.h"
#include "../actions/ActAddLabel.h"
#include "../actions/ActRemoveLabel.h"
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
				if (!commodity->offerFactions.contains(faction.factionId))
					continue;

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

	static uint GenerateMissionForClient(const uint clientId, const uint startBaseId)
	{
		uint shipArchetypeId = 0;
		pub::Player::GetShipID(clientId, shipArchetypeId);
		if (!shipArchetypeId)
			return 0;

		const auto& baseOffers = offersByBaseId.find(startBaseId);
		if (baseOffers == offersByBaseId.end())
			return 0;
		std::vector<BaseOffer> shuffledBaseFactions(offersByBaseId.at(startBaseId));
		std::ranges::shuffle(shuffledBaseFactions, rd);

		const RandomDestination& destination = GetRandomDestination(clientId, startBaseId, shipArchetypeId);
		if (!destination.commodity)
			return 0;

		uint missionId = 1;
		for (const auto& mission : Missions::missions)
			missionId = max(missionId, mission.second.id + 1);
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

		const uint InitialPlayer = CreateID("initial_player");
		const uint Players = CreateID("players");

		const std::string commsGotoWayPointTriggerName = "commsGotoWaypoint";
		const uint commsGotoWayPointTriggerId = CreateID(commsGotoWayPointTriggerName.c_str());
		/* Comms Goto Waypoint */
		{
			mission.triggers.try_emplace(commsGotoWayPointTriggerId, commsGotoWayPointTriggerName, commsGotoWayPointTriggerId, missionId, false, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(commsGotoWayPointTriggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndTimer(Missions::ConditionParent(missionId, commsGotoWayPointTriggerId), 5.0f, 0.0f));

			{
				Missions::ActEtherComm action;
				action.receiverObjNameOrLabel = Missions::Activator;
				action.id = CreateID("msnGoToTarget");
				action.senderVoiceId = CreateID("mc_leg_m01");
				action.senderIdsName = 13015;
				action.costume = offerFaction.missionCommission;
				action.lines = std::vector<uint>({ CreateID("rmb_targetatwaypoint_01-") });
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActEtherComm(action)));
			}
		}

		const uint checkForInitialInSpaceComms = CreateID("checkForInitialInSpaceComms");
		/* Set Objective */
		{
			const std::string triggerName = "setObjective";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			{
				Missions::ActSetNNObj action;
				action.label = Players;
				action.systemId = destination.targetSystemId;
				action.targetObjName = destination.targetObjId;
				action.message = FmtStr(524392, 0);
				action.message.append_good(commodity.id);
				action.message.append_base(destination.targetBaseId);
				action.position = destination.targetPosition;
				action.bestRoute = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActSetNNObj(action)));
			}

			{
				Missions::ActPlayNN action;
				action.label = InitialPlayer;
				action.soundIds = std::vector<uint>({ CreateID("cmsn_accepted") });
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActPlayNN(action)));
			}

			{
				Missions::ActAddLabel action;
				action.objNameOrLabel = Players;
				action.label = checkForInitialInSpaceComms;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAddLabel(action)));
			}
		}

		/* Cause initial comms for players already in space */
		{
			const std::string triggerName = "initialCommsInSpace";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndInSpace(Missions::ConditionParent(missionId, triggerId), checkForInitialInSpaceComms, {}));

			{
				Missions::ActRemoveLabel action;
				action.objNameOrLabel = Missions::Activator;
				action.label = checkForInitialInSpaceComms;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActRemoveLabel(action)));
			}

			{
				Missions::ActActTrig action;
				action.triggers.push_back({ commsGotoWayPointTriggerId, 1.0f });
				action.activate = true;
				action.branching = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActActTrig(action)));
			}
		}

		/* Remove initial comms label for any player who was on a base and is just launching. */
		{
			const std::string triggerName = "initialCommsBaseExitLabelRemoval";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndBaseExit(Missions::ConditionParent(missionId, triggerId), checkForInitialInSpaceComms, {}));

			{
				Missions::ActRemoveLabel action;
				action.objNameOrLabel = Missions::Activator;
				action.label = checkForInitialInSpaceComms;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActRemoveLabel(action)));
			}
		}

		/* Joining the Group of the Mission Players */
		{
			const std::string triggerName = "joinGroup";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndJoinGroup(Missions::ConditionParent(missionId, triggerId), 0));

			{
				Missions::ActAddLabel action;
				action.objNameOrLabel = Missions::Activator;
				action.label = Players;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAddLabel(action)));
			}

			{
				Missions::ActSetNNObj action;
				action.label = Missions::Activator;
				action.systemId = destination.targetSystemId;
				action.targetObjName = destination.targetObjId;
				action.message = FmtStr(524392, 0);
				action.message.append_good(commodity.id);
				action.message.append_base(destination.targetBaseId);
				action.position = destination.targetPosition;
				action.bestRoute = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActSetNNObj(action)));
			}

			{
				Missions::ActActTrig action;
				action.triggers.push_back({ commsGotoWayPointTriggerId, 1.0f });
				action.branching = true;
				action.activate = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActActTrig(action)));
			}
		}

		/* Leaving the Group of the Mission Players */
		{
			const std::string triggerName = "leaveGroup";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLeaveGroup(Missions::ConditionParent(missionId, triggerId), 0));

			{
				Missions::ActLeaveMsn action;
				action.label = Missions::Activator;
				action.leaveType = Missions::LeaveMsnType::Silent;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActLeaveMsn(action)));
			}
		}

		/* Launch from Any Base */
		{
			const std::string triggerName = "launchFromAnyBase";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLaunchComplete(Missions::ConditionParent(missionId, triggerId), Players, {}));

			// First cancel the branch in case it wasn't yet executed.
			{
				Missions::ActActTrig action;
				action.triggers.push_back({ commsGotoWayPointTriggerId, 1.0f });
				action.branching = true;
				action.activate = false;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActActTrig(action)));
			}
			// Re-add the branch to restart it.
			{
				Missions::ActActTrig action;
				action.triggers.push_back({ commsGotoWayPointTriggerId, 1.0f });
				action.activate = true;
				action.branching = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActActTrig(action)));
			}
		}

		/* Add Cargo */
		{
			const std::string triggerName = "addCargo";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndTimer(Missions::ConditionParent(missionId, triggerId), 1.0f, 0.0f));

			{
				Missions::ActAddCargo action;
				action.label = InitialPlayer;
				action.itemId = commodity.id;
				action.count = 1;
				action.missionFlagged = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAddCargo(action)));
			}
		}

		/* Initial Player Death */
		{
			const std::string triggerName = "death";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndDestroyed(Missions::ConditionParent(missionId, triggerId), InitialPlayer, Missions::CndDestroyed::DestroyCondition::Explode, 0, 1, false));

			{
				Missions::ActEtherComm action;
				action.receiverObjNameOrLabel = Players;
				action.id = CreateID("msnLostLoot");
				action.senderVoiceId = CreateID("mc_leg_m01");
				action.senderIdsName = 13015;
				action.costume = offerFaction.missionCommission;
				action.lines = std::vector<uint>({ CreateID("rmb_fail_destroyedloot_02-") });
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActEtherComm(action)));
			}

			{
				Missions::ActAdjRep action;
				action.label = Players;
				action.groupId = mission.offer.group;
				action.change = 0.0f;
				action.reason = Empathies::ReputationChangeReason::MissionFailure;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAdjRep(action)));
			}

			{
				Missions::ActLeaveMsn action;
				action.label = Players;
				action.leaveType = Missions::LeaveMsnType::Failure;
				action.failureStringId = 13089;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActLeaveMsn(action)));
			}

			{
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActTerminateMsn()));
			}
		}

		/* Initial Player Leaves */
		{
			const std::string triggerName = "leavingMsnInitialPlayer";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLeaveMsn(Missions::ConditionParent(missionId, triggerId), InitialPlayer));

			{
				Missions::ActEtherComm action;
				action.receiverObjNameOrLabel = Players;
				action.id = CreateID("msnLeftLoot");
				action.senderVoiceId = CreateID("mc_leg_m01");
				action.senderIdsName = 13015;
				action.costume = offerFaction.missionCommission;
				action.lines = std::vector<uint>({ CreateID("rmb_fail_destroyedloot_02-") });
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActEtherComm(action)));
			}

			{
				Missions::ActRemoveCargo action;
				action.label = Missions::Activator;
				action.itemId = commodity.id;
				action.count = 1;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActRemoveCargo(action)));
			}

			{
				Missions::ActAdjRep action;
				action.label = Players;
				action.groupId = mission.offer.group;
				action.change = 0.0f;
				action.reason = Empathies::ReputationChangeReason::MissionAbortion;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAdjRep(action)));
			}

			{   // The initial player might be entirely removed from the mission at this point. Use Activator instead.
				Missions::ActLeaveMsn action;
				action.label = Missions::Activator;
				action.leaveType = Missions::LeaveMsnType::Failure;
				action.failureStringId = 13086;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActLeaveMsn(action)));
			}

			{
				Missions::ActLeaveMsn action;
				action.label = Players;
				action.leaveType = Missions::LeaveMsnType::Failure;
				action.failureStringId = 13086;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActLeaveMsn(action)));
			}

			{
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActTerminateMsn()));
			}
		}

		/* Any Player Leaves */
		{
			const std::string triggerName = "leavingMsnPlayers";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndLeaveMsn(Missions::ConditionParent(missionId, triggerId), Players));

			{
				Missions::ActLeaveMsn action;
				action.label = Missions::Activator;
				action.leaveType = Missions::LeaveMsnType::Failure;
				action.failureStringId = 13086;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActLeaveMsn(action)));
			}

			{
				Missions::ActLeaveGroup action;
				action.label = Missions::Activator;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActLeaveGroup(action)));
			}
		}

		/* Land on Target Base */
		{
			const std::string triggerName = "landOnTargetBase";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Off);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndBaseEnter(Missions::ConditionParent(missionId, triggerId), InitialPlayer, { destination.targetBaseId }));
			// No check follows if we actually have the cargo in bay. If it would be lost before, the mission would've failed anyway.

			{
				Missions::ActRemoveCargo action;
				action.label = InitialPlayer;
				action.itemId = commodity.id;
				action.count = 1;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActRemoveCargo(action)));
			}

			{
				Missions::ActAdjRep action;
				action.label = Players;
				action.groupId = mission.offer.group;
				action.change = reputation;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAdjRep(action)));
			}

			{
				Missions::ActAdjAcct action;
				action.label = Players;
				action.cash = mission.offer.reward;
				action.splitBetweenPlayers = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAdjAcct(action)));
			}

			{
				Missions::ActEtherComm action;
				action.receiverObjNameOrLabel = Players;
				action.id = CreateID("msnCargoDelivered");
				action.senderVoiceId = CreateID("mc_leg_m01");
				action.senderIdsName = 13015;
				action.costume = offerFaction.missionCommission;
				action.lines = std::vector<uint>({ CreateID("rmb_success_returnloot_01-") });
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActEtherComm(action)));
			}

			{
				Missions::ActPlayNN action;
				action.label = InitialPlayer;
				action.soundIds = std::vector<uint>({ CreateID("mission_complete") });
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActPlayNN(action)));
			}

			{
				Missions::ActLeaveMsn action;
				action.label = Players;
				action.leaveType = Missions::LeaveMsnType::Success;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActLeaveMsn(action)));
			}

			{
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActTerminateMsn()));
			}
		}

		/* Land on Any Base */
		{
			const std::string triggerName = "landOnAnyBase";
			const uint triggerId = CreateID(triggerName.c_str());
			mission.triggers.try_emplace(triggerId, triggerName, triggerId, missionId, true, Missions::Trigger::TriggerRepeatable::Auto);
			Missions::Trigger& trigger = mission.triggers.at(triggerId);

			trigger.condition = Missions::ConditionPtr(new Missions::CndBaseEnter(Missions::ConditionParent(missionId, triggerId), InitialPlayer, {}));

			// Remove cargo because Freelancer does not transfer MISSION CARGO flag
			{
				Missions::ActRemoveCargo action;
				action.label = InitialPlayer;
				action.itemId = commodity.id;
				action.count = 1;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActRemoveCargo(action)));
			}

			// Re-add cargo to keep it visible as MISSION CARGO flagged item
			{
				Missions::ActAddCargo action;
				action.label = InitialPlayer;
				action.itemId = commodity.id;
				action.count = 1;
				action.missionFlagged = true;
				trigger.actions.push_back(Missions::ActionPtr(new Missions::ActAddCargo(action)));
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
					const uint missionId = GenerateMissionForClient(clientId, baseId);
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

			void __stdcall ReqShipArch_After(unsigned int shiparchId, unsigned int clientId)
			{
				const auto missionIds(missionIdsByClientId[clientId]);
				for (const uint missionId : missionIds)
				{
					const auto& missionEntry = Missions::missions.find(missionId);
					const auto& allowedShipArchetypeIds = missionEntry->second.offer.shipArchetypeIds;
					if (!allowedShipArchetypeIds.empty() && !allowedShipArchetypeIds.contains(shiparchId))
					{
						Missions::missions.erase(missionEntry);
						missionIdsByClientId.at(clientId).erase(missionId);
					}
				}
				if (missionIdsByClientId.at(clientId).empty())
					missionIdsByClientId.erase(clientId);

				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}