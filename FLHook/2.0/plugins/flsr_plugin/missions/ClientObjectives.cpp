#include "ClientObjectives.h"
#include "BestPath.h"
#include "Mission.h"

namespace Missions
{
	namespace ClientObjectives
	{
		std::unordered_map<uint, Objective> objectiveByClientId;
		std::unordered_map<uint, std::vector<pub::Player::MissionObjective>> objectivesByClientId;

		void DeleteClientObjectives(const uint clientId, const uint missionId)
		{
			if (missionId > 0)
			{
				if (const auto& entry = objectiveByClientId.find(clientId); entry == objectiveByClientId.end() || entry->second.missionId != missionId)
					return;
			}
			objectiveByClientId.erase(clientId);
			objectivesByClientId.erase(clientId);
		}

		bool DoesClientHaveObjective(const uint clientId)
		{
			return objectiveByClientId.contains(clientId);
		}

		void SetClientObjective(const uint clientId, const Objective objective)
		{
			DeleteClientObjectives(clientId, 0);
			objectiveByClientId[clientId] = objective;
		}

		void SendClientObjectives(const uint clientId)
		{
			const auto& objectiveEntry = objectiveByClientId.find(clientId);
			if (objectiveEntry == objectiveByClientId.end())
				return;

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
					bestPath.waypointCount = 1;
					bestPath.entries[0] = destination;
				}

				// Players cannot have player-waypoints while being in a mission. So we can assume this always takes priority.
				Server.RequestBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
			}
			else
			{
				// Just for Display message stuff without waypoints
			}
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

		bool __stdcall Send_FLPACKET_COMMON_REQUEST_BEST_PATH(uint clientId, const XRequestBestPath& data, int size)
		{
			// Players can have player-waypoints only when NOT in a mission. So we can safely assume this works as intended.
			if (!objectiveByClientId.contains(clientId))
			{
				returncode = DEFAULT_RETURNCODE;
				return true;
			}

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;

			// Early exit if no "best path" was generated or the character has somehow left by now.
			if (data.waypointCount == 0 || !HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId)) 
				return false;

			const auto& missionEntry = missions.find(objectiveByClientId[clientId].missionId);
			if (missionEntry == missions.end())
				return false;

			std::vector<pub::Player::MissionObjective> objectives;
			objectives.reserve(data.waypointCount + 2); // +1 for potential LAUNCH objective; +1 for potential MAIN objective
			const int objectiveIndexOffset = AppendLaunchToSpaceObjectiveIfNecessary(clientId, objectives) ? 1 : 0;
			objectives.resize(data.waypointCount + objectiveIndexOffset);

			// The first "best path" waypoint is being treated as objective to reach the actual destination.
			auto& nextObjective = objectives[objectiveIndexOffset];
			if (data.entries[0].objId > 0)
			{
				IObjRW* inspect;
				StarSystem* system;
				uint objId = data.entries[0].objId;
				if (GetShipInspect(objId, inspect, system))
				{
					nextObjective.type = pub::Player::MissionObjectiveType::ObjectiveWaypoint;
					if (inspect->cobj->type & ObjectType::TradelaneRing)
					{
						if (data.entries[data.waypointCount - 1].objId > 0 && GetShipInspect(objId, inspect, system) && (inspect->cobj->type & ObjectType::JumpGate))
						{
							nextObjective.message = FmtStr(13071, 0);
							nextObjective.message.append_system(reinterpret_cast<CSolar*>(inspect->cobj)->jumpDestSystem);
						}
						else
						{
							nextObjective.message = FmtStr(13060, 0);
						}
					}
					else if (inspect->cobj->type & ObjectType::JumpGate)
					{
						nextObjective.message = FmtStr(13080, 0);
						nextObjective.message.append_system(reinterpret_cast<CSolar*>(inspect->cobj)->jumpDestSystem);
					}
				}
			}

			const auto& playerObjective = objectiveByClientId[clientId];
			const auto& lastWaypoint = data.entries[data.waypointCount - 1];
			const bool lastObjectiveIsMainObjective = lastWaypoint.systemId == playerObjective.systemId && 
													  lastWaypoint.position.x == playerObjective.position.x &&
													  lastWaypoint.position.y == playerObjective.position.y &&
													  lastWaypoint.position.z == playerObjective.position.z;

			const pub::Player::MissionObjectiveType lastObjectiveType = (pub::Player::MissionObjectiveType)((uint)pub::Player::MissionObjectiveType::ObjectiveWaypoint | (uint)pub::Player::MissionObjectiveType::MissionText | (uint)pub::Player::MissionObjectiveType::ActiveLog);

			if (lastObjectiveIsMainObjective)
			{
				auto& lastObjective = objectives[objectives.size() - 1];
				lastObjective.type = lastObjectiveType;
				lastObjective.message = FmtStr(playerObjective.message, 0);
			}

			// Translate all "best path" waypoints to nav map markers the objectives.
			for (int waypointIndex = 0, objectiveIndex = objectiveIndexOffset; waypointIndex < data.waypointCount; waypointIndex++, objectiveIndex++)
			{
				FmtStr::NavMarker marker;
				marker.pos = data.entries[waypointIndex].position;
				marker.system = data.entries->systemId;
				objectives[objectiveIndex].message.append_nav_marker(marker);
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

			// Check whether the client needs a packet with new objectives. This must be done to ensure constant re-generation of best path will not flood the network.
			if (!AreObjectivesEqual(objectivesByClientId[clientId], objectives))
			{
				// Save the current objectives for later comparison.
				objectivesByClientId[clientId] = objectives;

				FmtStr missionTitle(missionEntry->second.offer.title, 0);
				FmtStr missionDescription(missionEntry->second.offer.description, 0);
				pub::Player::SetMissionObjectives(clientId, 12, objectives.data(), objectives.size(), missionTitle, 2, missionDescription);
			}

			return true;
		}

		float elapsedObjectivesTime = 0.0f;
		void __stdcall Elapse_Time_AFTER(float seconds)
		{
			returncode = DEFAULT_RETURNCODE;

			elapsedObjectivesTime += seconds;
			if (elapsedObjectivesTime > 2.0f)
			{
				elapsedObjectivesTime = 0;
				struct PlayerData* playerData = nullptr;
				while (playerData = Players.traverse_active(playerData))
				{
					if (!objectiveByClientId.contains(playerData->iOnlineID) || HkIsInCharSelectMenu(playerData->iOnlineID))
						continue;

					bool inTradelane = false;
					IObjRW* inspect;
					StarSystem* starSystem;
					if (playerData->iShipID && GetShipInspect(playerData->iShipID, inspect, starSystem) && inspect->is_using_tradelane(&inTradelane) == 0 && !inTradelane)
						SendClientObjectives(playerData->iOnlineID);
				}
			}
		}
	}
}