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
				pub::Player::SetMissionObjectives(clientId, missionId, nullptr, 0, FmtStr(0, 0), 0, FmtStr(0, 0));
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
			if(!AreObjectivesEqual(computedObjectivesByClientId[clientId], objectives))
			{
				// Save the current objectives for later comparison.
				computedObjectivesByClientId[clientId] = objectives;

				FmtStr missionTitle(missionOffer.title, 0);
				FmtStr missionDescription(missionOffer.description, 0);
				pub::Player::SetMissionObjectives(clientId, missionId, objectives.data(), objectives.size(), missionTitle, 0, missionDescription);
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

			intermediateDestination.systemId = 0;

			if (objectiveEntry->second.systemId)
			{
				// Define destination location
				XRequestBestPathEntry destination;
				uint playerSystemId;
				pub::Player::GetSystem(clientId, playerSystemId);
				const bool playerNotInDestinationSystem = playerSystemId != objectiveEntry->second.systemId;
				if (playerNotInDestinationSystem)
				{
					destination = BestPath::GetJumpObjectToNextSystem(clientId, objectiveEntry->second.systemId);
				}
				else
				{
					destination.systemId = objectiveEntry->second.systemId;
					destination.position = objectiveEntry->second.position;
					destination.objId = objectiveEntry->second.objId;
				}

				// Tell the server to compute the best path
				XRequestBestPath bestPath;
				bestPath.noPathFound = false;
				bestPath.repId = 0;
				if (playerNotInDestinationSystem || objectiveEntry->second.bestPath)
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
				const auto& missionEntry = missions.find(objectiveByClientId[clientId].missionId);
				if (missionEntry != missions.end())
				{
					pub::Player::MissionObjective objective;
					objective.message = FmtStr(objectiveEntry->second.message, 0);
					objective.type = pub::Player::MissionObjectiveType::MissionText | pub::Player::MissionObjectiveType::SimpleEntry | pub::Player::MissionObjectiveType::ActiveLog;
					SetObjectives(clientId, objectiveEntry->second.missionId, missionEntry->second.offer, std::vector<pub::Player::MissionObjective>({ objective }));
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
					uint objId = base.iObjectID;
					IObjRW* inspect;
					StarSystem* starSystem;
					if (GetShipInspect(objId, inspect, starSystem))
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

		static bool HasPlayerPassedEntranceTradelaneRing(const uint clientId, const XRequestBestPath& route)
		{
			IObjRW* inspect;
			StarSystem* system;
			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			if (route.waypointCount > 1 && route.entries[0].objId && route.entries[1].objId && // There must be at least 2 waypoints with ObjIds to form a tradelane.
				shipId && GetShipInspect(shipId, inspect, system) && inspect->cobj->objectClass == CObject::CSHIP_OBJECT && static_cast<CShip*>(inspect->cobj)->is_using_tradelane())
			{
				uint currentTLRId = route.entries[0].objId; // Potential start of the current TLR.
				uint targetTLRId = route.entries[1].objId; // Potential end of the current TLR.
				if (GetShipInspect(targetTLRId, inspect, system) && inspect->cobj->type & ObjectType::TradelaneRing &&
					GetShipInspect(currentTLRId, inspect, system) && inspect->cobj->type & ObjectType::TradelaneRing)
				{
					const CSolar* currentTLR = static_cast<CSolar*>(inspect->cobj);
					uint nextTLRId = currentTLR->get_next_trade_ring();
					while (nextTLRId)
					{
						if (nextTLRId == targetTLRId)
							return true;
						else if (GetShipInspect(nextTLRId, inspect, system) && inspect->cobj->type & ObjectType::TradelaneRing)
							nextTLRId = static_cast<CSolar*>(inspect->cobj)->get_next_trade_ring();
						else
							break;
					}
					uint previousTLRId = currentTLR->get_prev_trade_ring();
					while (previousTLRId)
					{
						if (previousTLRId == targetTLRId)
							return true;
						else if (GetShipInspect(previousTLRId, inspect, system) && inspect->cobj->type & ObjectType::TradelaneRing)
							previousTLRId = static_cast<CSolar*>(inspect->cobj)->get_prev_trade_ring();
						else
							break;
					}
				}
			}
			return false;
		}

		static bool IsJumpObject(const IObjRW* inspect)
		{
			return (inspect->cobj->type & ObjectType::JumpGate) || (inspect->cobj->type & ObjectType::JumpHole) || (inspect->cobj->type & ObjectType::AirlockGate);
		}

		bool __stdcall Send_FLPACKET_COMMON_REQUEST_BEST_PATH(uint clientId, const XRequestBestPath& data, int size)
		{
			// Players can have player-waypoints only when NOT in a mission. So we can safely assume this works as intended.
			if (!objectiveByClientId.contains(clientId))
			{
				returncode = DEFAULT_RETURNCODE;
				return true;
			}

			// Early exit if no "best path" was generated or the character has somehow left by now.
			if (data.waypointCount == 0 || !HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			{
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const auto& missionEntry = missions.find(objectiveByClientId[clientId].missionId);
			if (missionEntry == missions.end())
			{
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			// Apply object ID for last waypoint because FL drops that on computed paths.
			auto& lastWaypoint = ((XRequestBestPathEntry*)data.entries)[data.waypointCount - 1];
			if (lastWaypoint.systemId == intermediateDestination.systemId &&
				lastWaypoint.position.x == intermediateDestination.position.x &&
				lastWaypoint.position.y == intermediateDestination.position.y &&
				lastWaypoint.position.z == intermediateDestination.position.z)
			{
				lastWaypoint.objId = intermediateDestination.objId;
			}

			bool preventSendingMissionMessage = false;
			if (HasPlayerPassedEntranceTradelaneRing(clientId, data))
			{
				preventSendingMissionMessage = true;
				// Remove the first TLR from the waypoints list because it was just entered.
				XRequestBestPath& temp = (XRequestBestPath&)data;
				temp.waypointCount--;
				for (int index = 0; index < temp.waypointCount; index++)
					temp.entries[index] = temp.entries[index + 1];
			}

			std::vector<pub::Player::MissionObjective> objectives;
			objectives.reserve(data.waypointCount + 2); // +1 for potential LAUNCH objective; +1 for potential MAIN objective.
			const size_t objectiveIndexOffset = AppendLaunchToSpaceObjectiveIfNecessary(clientId, objectives) ? 1 : 0;
			objectives.resize(data.waypointCount + objectiveIndexOffset);

			// The first "best path" waypoint is being treated as objective to reach the actual destination.
			auto& nextObjective = objectives[objectiveIndexOffset];
			if (data.entries[0].objId != 0)
			{
				IObjRW* inspect;
				StarSystem* system;
				uint objId = data.entries[0].objId;
				if (GetShipInspect(objId, inspect, system))
				{
					nextObjective.type = pub::Player::MissionObjectiveType::ObjectiveWaypoint;
					if (!preventSendingMissionMessage)
						nextObjective.type = nextObjective.type | pub::Player::MissionObjectiveType::MissionText;

					if (inspect->cobj->type & ObjectType::TradelaneRing)
					{
						if (data.entries[data.waypointCount - 1].objId > 0 && GetShipInspect(objId, inspect, system) && IsJumpObject(inspect))
						{
							nextObjective.message = FmtStr(13071, 0);
							nextObjective.message.append_system(reinterpret_cast<CSolar*>(inspect->cobj)->jumpDestSystem);
						}
						else
						{
							nextObjective.message = FmtStr(13060, 0);
						}
					}
					else if (IsJumpObject(inspect))
					{
						nextObjective.message = FmtStr(13080, 0);
						nextObjective.message.append_system(reinterpret_cast<CSolar*>(inspect->cobj)->jumpDestSystem);
					}
				}
			}

			const auto& playerObjective = objectiveByClientId[clientId];
			const bool lastObjectiveIsMainObjective = lastWaypoint.systemId == playerObjective.systemId && 
													  lastWaypoint.position.x == playerObjective.position.x &&
													  lastWaypoint.position.y == playerObjective.position.y &&
													  lastWaypoint.position.z == playerObjective.position.z;

			uint lastObjectiveType = pub::Player::MissionObjectiveType::ObjectiveWaypoint | pub::Player::MissionObjectiveType::ActiveLog;
			if (!preventSendingMissionMessage)
				lastObjectiveType= lastObjectiveType | pub::Player::MissionObjectiveType::MissionText;

			if (lastObjectiveIsMainObjective)
			{
				auto& lastObjective = objectives[objectives.size() - 1];
				lastObjective.type = lastObjectiveType;
				lastObjective.message = FmtStr(playerObjective.message, 0);
			}

			// Translate all "best path" waypoints to nav map markers the objectives.
			for (size_t waypointIndex = 0, objectiveIndex = objectiveIndexOffset; waypointIndex < data.waypointCount; waypointIndex++, objectiveIndex++)
			{
				const auto& waypoint = data.entries[waypointIndex];
				FmtStr::NavMarker marker;
				marker.pos = waypoint.position;
				marker.system = waypoint.systemId;
				objectives[objectiveIndex].message.append_nav_marker(marker);
				if (waypoint.objId)
					objectives[objectiveIndex].message.append_rep_instance(waypoint.objId);
			}

			if (!lastObjectiveIsMainObjective)
			{
				pub::Player::MissionObjective finalObjective;
				finalObjective.type = lastObjectiveType;
				finalObjective.message = FmtStr(playerObjective.message, 0);
				FmtStr::NavMarker marker;
				marker.pos = playerObjective.position;
				marker.system = playerObjective.systemId;
				finalObjective.message.append_nav_marker(marker);
				objectives.push_back(finalObjective);
			}

			SetObjectives(clientId, playerObjective.missionId, missionEntry->second.offer, objectives);

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
			ComputeAndSendClientObjectives(clientId);
			returncode = DEFAULT_RETURNCODE;
		}

		void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
		{
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