#include "ClientObjectives.h"
#include "BestPath.h"
#include "Mission.h"

namespace Missions
{
	namespace ClientObjectives
	{
		std::unordered_map<uint, Objective> objectiveByClientId;
		std::unordered_map<uint, std::vector<pub::Player::MissionObjective>> computedObjectivesByClientId;

		void ClearClientObjectives(const uint clientId, const uint missionId)
		{
			if (missionId > 0)
			{
				if (const auto& entry = objectiveByClientId.find(clientId); entry == objectiveByClientId.end() || entry->second.missionId != missionId)
					return;
				pub::Player::SetMissionObjectives(clientId, missionId, nullptr, 0, FmtStr(1, 0), 0, FmtStr(1, 0));
			}

			objectiveByClientId.erase(clientId);
			computedObjectivesByClientId.erase(clientId);
		}

		bool DoesClientHaveObjective(const uint clientId)
		{
			return objectiveByClientId.contains(clientId);
		}

		static bool AreObjectivesEqual(const std::vector<pub::Player::MissionObjective>& objectivesA, const std::vector<pub::Player::MissionObjective>& objectivesB)
		{
			if (objectivesA.size() != objectivesB.size())
				return false;

			for (size_t index = 0, length = objectivesB.size(); index < length; index++)
			{
				if (objectivesA[index].type != objectivesB[index].type)
					return false;

				const size_t bufferSize = 1024;
				char flatFmtStrA[bufferSize];
				const size_t sizeA = objectivesA[index].message.flatten(flatFmtStrA, bufferSize);
				char flatFmtStrB[bufferSize];
				const size_t sizeB = objectivesB[index].message.flatten(flatFmtStrB, bufferSize);
				if (sizeA != sizeB || std::memcmp(flatFmtStrA, flatFmtStrB, sizeA) != 0)
					return false;
			}

			return true;
		}

		void static SetObjectives(const uint clientId, const uint missionId, const MissionOffer& missionOffer, const std::vector<pub::Player::MissionObjective>& objectives)
		{
			if (!AreObjectivesEqual(computedObjectivesByClientId[clientId], objectives))
			{
				// Save the current objectives for later comparison.
				computedObjectivesByClientId[clientId] = objectives;

				FmtStr missionTitle(missionOffer.title, 0);

				FmtStr message(327681, 0);
				FmtStr header(327682, 0);
				header.append_int(missionOffer.reward);
				message.append_fmt_str(header);
				message.append_fmt_str(missionOffer.description);

				pub::Player::SetMissionObjectives(clientId, missionId, objectives.data(), objectives.size(), missionTitle, 0, message);
			}
		}

		/*
		   The final computed best path waypoint never contains the Object Id of the target object.
		   Save it here from the original request to re-apply it afterwards.
		*/
		XRequestBestPathEntry intermediateDestination;

		static void ComputeAndSendClientObjectives(const uint clientId)
		{
			const auto& objectiveEntry = objectiveByClientId.find(clientId);
			if (objectiveEntry == objectiveByClientId.end())
				return;

			const auto& mainObjective = objectiveEntry->second;

			intermediateDestination.systemId = 0;

			if (mainObjective.systemId)
			{
				// Define destination location
				XRequestBestPathEntry destination;
				uint playerSystemId;
				pub::Player::GetSystem(clientId, playerSystemId);
				const bool playerNotInDestinationSystem = playerSystemId != mainObjective.systemId;
				if (playerNotInDestinationSystem)
				{
					destination = BestPath::GetJumpObjectToNextSystem(clientId, mainObjective.systemId);
				}
				else
				{
					destination.systemId = mainObjective.systemId;
					destination.position = mainObjective.position;
					destination.objId = mainObjective.objId;
				}

				// Tell the server to compute the best path
				XRequestBestPath bestPath;
				bestPath.noPathFound = false;
				bestPath.repId = 0;
				if (playerNotInDestinationSystem || mainObjective.bestPath)
				{
					// Define start location
					XRequestBestPathEntry start;
					uint playerObjectId = 0;
					uint baseId;
					pub::Player::GetBase(clientId, baseId);
					if (baseId)
					{
						for (const auto& base : lstBases)
						{
							if (base.iBaseID == baseId && base.iObjectID)
							{
								playerObjectId = base.iObjectID;
								break;
							}
						}
					}
					else
					{
						pub::Player::GetShip(clientId, playerObjectId);
					}

					if (!playerObjectId)
						return;

					IObjRW* inspect;
					StarSystem* starSystem;
					if (GetShipInspect(playerObjectId, inspect, starSystem))
					{
						start.systemId = inspect->cobj->system;
						start.position = inspect->cobj->vPos;
						start.objId = inspect->cobj->id;
					}

					bestPath.waypointCount = 2;
					bestPath.entries[0] = start;
					bestPath.entries[1] = destination;
				}
				else
				{
					// To set a simple waypoint we still have to set a start and end. Make them the same to be a spot.
					bestPath.waypointCount = 2;
					bestPath.entries[0] = destination;
					bestPath.entries[1] = destination;
				}

				intermediateDestination = destination;
				// Players cannot have player-waypoints while being in a mission. So we can assume this always takes priority.
				Server.RequestBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
			}
			else
			{
				// Just to display messages without waypoints
				if (const auto& missionEntry = missions.find(mainObjective.missionId); missionEntry != missions.end())
				{
					pub::Player::MissionObjective objective;
					objective.message = mainObjective.message;
					objective.type = pub::Player::MissionObjectiveType::MissionText | pub::Player::MissionObjectiveType::SimpleEntry | pub::Player::MissionObjectiveType::ActiveLog;
					SetObjectives(clientId, mainObjective.missionId, missionEntry->second.offer, std::vector<pub::Player::MissionObjective>({ objective }));
				}
			}
		}

		static bool AppendLaunchToSpaceObjectiveIfNecessary(const uint clientId, std::vector<pub::Player::MissionObjective>& objectives)
		{
			uint baseId;
			pub::Player::GetBase(clientId, baseId);
			if (!baseId)
				return false;

			for (const auto& base : lstBases)
			{
				if (base.iBaseID == baseId && base.iObjectID)
				{
					pub::Player::MissionObjective objective;
					objective.type = pub::Player::MissionObjectiveType::MissionText;
					objective.message = FmtStr(13081, 0);
					objective.message.append_base(baseId);
					FmtStr::NavMarker marker;
					IObjRW* inspect;
					StarSystem* starSystem;
					if (GetShipInspect(base.iObjectID, inspect, starSystem))
					{
						marker.pos = inspect->cobj->vPos;
						marker.system = base.iSystemID;
					}
					else
					{
						marker.pos = Vector(0, 0, 0);
						uint systemId;
						pub::Player::GetSystem(clientId, systemId);
						marker.system = systemId;
					}
					objective.message.append_nav_marker(marker);
					objectives.push_back(objective);
					return true;
				}
			}
			return false;
		}

		static bool IsJumpObject(const IObjRW* inspect)
		{
			return (inspect->cobj->type & ObjectType::JumpGate) || (inspect->cobj->type & ObjectType::JumpHole) || (inspect->cobj->type & ObjectType::AirlockGate);
		}

		static bool IsPlayerInTradelane(const uint clientId)
		{
			IObjRW* inspect;
			StarSystem* system;
			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			return shipId && GetShipInspect(shipId, inspect, system) && inspect->cobj->objectClass == CObject::CSHIP_OBJECT && static_cast<CShip*>(inspect->cobj)->is_using_tradelane();
		}

		static bool CreateIntermediateObjective(const XRequestBestPath& route, pub::Player::MissionObjective& objective, const bool playerInTLR, const bool noMissionText)
		{
			if (route.waypointCount == 0)
				return false;

			const auto& waypoint = route.entries[0];
			const uint objId = waypoint.objId;
			if (objId == 0)
				return false;

			IObjRW* inspect;
			StarSystem* system;
			if (!GetShipInspect(objId, inspect, system))
				return false;

			if (playerInTLR)
				objective.type = pub::Player::MissionObjectiveType::SimpleEntry;
			else
				objective.type = pub::Player::MissionObjectiveType::ObjectiveWaypoint;
			if (!noMissionText)
				objective.type |= pub::Player::MissionObjectiveType::MissionText;

			const auto& obj = inspect->cobj;
			bool result = false;
			if (obj->type & ObjectType::TradelaneRing)
			{
				const uint lastObjId = route.entries[route.waypointCount - 1].objId;
				if (lastObjId && GetShipInspect(lastObjId, inspect, system) && IsJumpObject(inspect))
				{
					objective.message = FmtStr(13071, 0);
					objective.message.append_system(reinterpret_cast<CSolar*>(inspect->cobj)->jumpDestSystem);
					result = true;
				}
				else
				{
					objective.message = FmtStr(13060, 0);
					result = true;
				}
			}
			else if (IsJumpObject(inspect))
			{
				objective.message = FmtStr(13080, 0);
				objective.message.append_system(reinterpret_cast<CSolar*>(obj)->jumpDestSystem);
				result = true;
			}

			if (result && !playerInTLR)
			{
				FmtStr::NavMarker marker;
				marker.pos = waypoint.position;
				marker.system = waypoint.systemId;
				objective.message.append_nav_marker(marker);
				objective.message.append_rep_instance(objId);
			}

			return result;
		}

		bool static IsObjectiveWaypoint(const Missions::ClientObjectives::Objective& objective, const XRequestBestPathEntry& waypoint)
		{
			return	waypoint.systemId   == objective.systemId &&
					waypoint.position.x == objective.position.x &&
					waypoint.position.y == objective.position.y &&
					waypoint.position.z == objective.position.z;
		}

		bool __stdcall Send_FLPACKET_COMMON_REQUEST_BEST_PATH(uint clientId, const XRequestBestPath& data, int size)
		{
			const auto& objectiveByClientEntry = objectiveByClientId.find(clientId);
			// Players can have player-waypoints only when NOT in a mission. So we can safely assume this works as intended.
			if (objectiveByClientEntry == objectiveByClientId.end())
			{
				returncode = DEFAULT_RETURNCODE;
				return true;
			}
			const auto& mainObjective = objectiveByClientEntry->second;

			// Early exit the character has somehow left by now.
			if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			{
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			// If there is no more mission existing behind this objective, ignore it.
			const auto& missionEntry = missions.find(mainObjective.missionId);
			if (missionEntry == missions.end())
			{
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			// Apply object ID for last waypoint because FL drops that on computed paths.
			if (data.waypointCount > 0)
			{
				auto& lastWaypoint = ((XRequestBestPathEntry*)data.entries)[data.waypointCount - 1];
				if (lastWaypoint.systemId == intermediateDestination.systemId &&
					lastWaypoint.position.x == intermediateDestination.position.x &&
					lastWaypoint.position.y == intermediateDestination.position.y &&
					lastWaypoint.position.z == intermediateDestination.position.z)
				{
					lastWaypoint.objId = intermediateDestination.objId;
				}
			}
			
			const bool playerInTLR = IsPlayerInTradelane(clientId);

			std::vector<pub::Player::MissionObjective> objectives;
			objectives.reserve(data.waypointCount + 2); // +1 for potential LAUNCH objective; +1 for MAIN objective.

			const bool launchFromBaseObjectivePresent = AppendLaunchToSpaceObjectiveIfNecessary(clientId, objectives);

			pub::Player::MissionObjective intermediateObjective;
			const bool intermediateWaypointPresent = CreateIntermediateObjective(data, intermediateObjective, playerInTLR, playerInTLR || launchFromBaseObjectivePresent);
			// If the player is not in a TLR, we have a normal main objective. Put it as next target.
			if (intermediateWaypointPresent && !playerInTLR)
				objectives.push_back(intermediateObjective);

			// Set navigation points for each waypoint
			for (size_t waypointIndex = intermediateWaypointPresent ? 1 : 0; waypointIndex < data.waypointCount; waypointIndex++)
			{
				const auto& waypoint = data.entries[waypointIndex];
				FmtStr::NavMarker marker;
				marker.pos = waypoint.position;
				marker.system = waypoint.systemId;
				auto& objective = objectives.emplace_back(pub::Player::MissionObjectiveType::IntermediateWaypoint, FmtStr(1, 0));
				objective.message.append_nav_marker(marker);
				if (waypoint.objId)
					objective.message.append_rep_instance(waypoint.objId);
			}

			// If the player is in a TLR, a simple entry without navigation point will be created. This must be put after all navigation points.
			if (intermediateWaypointPresent && playerInTLR)
				objectives.push_back(intermediateObjective);

			// Always append the Main Objective as extra objective.
			// Freelancer has a bug where it requires always to have one more intermediate waypoint before the last objective,
			// or it will not draw lines correctly on the map, or not allow clicking the target object and properly connecting it to the real object for the player.
			uint lastObjectiveType = pub::Player::MissionObjectiveType::ObjectiveWaypoint | pub::Player::MissionObjectiveType::ActiveLog;
			if (!playerInTLR && (!intermediateWaypointPresent || launchFromBaseObjectivePresent))
				lastObjectiveType |= pub::Player::MissionObjectiveType::MissionText;
			auto& lastObjective = objectives.emplace_back(lastObjectiveType, mainObjective.message);
			FmtStr::NavMarker marker;
			marker.pos = mainObjective.position;
			marker.system = mainObjective.systemId;
			lastObjective.message.append_nav_marker(marker);
			if (mainObjective.objId)
				lastObjective.message.append_rep_instance(mainObjective.objId);

			SetObjectives(clientId, mainObjective.missionId, missionEntry->second.offer, objectives);

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		void SetClientObjective(const uint clientId, const Objective objective)
		{
			ClearClientObjectives(clientId, 0);
			objectiveByClientId[clientId] = objective;
			ComputeAndSendClientObjectives(clientId);
		}

		void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId)
		{
			// The player may have re-logged in with Character Menu. Erase the entry to make sure the objectives are sent again.
			computedObjectivesByClientId.erase(clientId);
			ComputeAndSendClientObjectives(clientId);
			returncode = DEFAULT_RETURNCODE;
		}

		void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
		{
			// The player may have re-logged in with Character Menu. Erase the entry to make sure the objectives are sent again.
			computedObjectivesByClientId.erase(clientId);
			ComputeAndSendClientObjectives(clientId);
			returncode = DEFAULT_RETURNCODE;
		}

		void __stdcall GoTradelane_AFTER(unsigned int clientId, const XGoTradelane& tradeLaneInfo)
		{
			ComputeAndSendClientObjectives(clientId);
			returncode = DEFAULT_RETURNCODE;
		}

		void __stdcall StopTradelane_AFTER(unsigned int clientId, unsigned int p2, unsigned int p3, unsigned int p4)
		{
			ComputeAndSendClientObjectives(clientId);
			returncode = DEFAULT_RETURNCODE;
		}

		float elapsedObjectivesTime = 0.0f;
		void __stdcall Elapse_Time_AFTER(float seconds)
		{
			elapsedObjectivesTime += seconds;
			if (elapsedObjectivesTime > 2.0f)
			{
				elapsedObjectivesTime = 0;
				struct PlayerData* playerData = nullptr;
				while (playerData = Players.traverse_active(playerData))
				{
					if (!DoesClientHaveObjective(playerData->iOnlineID) || HkIsInCharSelectMenu(playerData->iOnlineID))
						continue;

					bool inTradelane = false;
					IObjRW* inspect;
					StarSystem* starSystem;
					if (playerData->iShipID && GetShipInspect(playerData->iShipID, inspect, starSystem) && inspect->cobj->objectClass == CObject::CSHIP_OBJECT)
					{
						const CShip* ship = static_cast<CShip*>(inspect->cobj);
						if (!ship->is_jumping() && !ship->is_using_tradelane() && !ship->is_launching())
							ComputeAndSendClientObjectives(playerData->iOnlineID);
					}
				}
			}
			returncode = DEFAULT_RETURNCODE;
		}
	}
}