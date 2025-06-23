#include "ClientObjectives.h"
#include "BestPath.h"

namespace Missions
{
	namespace ClientObjectives
	{
		std::unordered_map<uint, Objective> objectiveByClientId;
		std::unordered_map<uint, std::vector<pub::Player::MissionObjective>> objectivesByClientId;
		uint bestPathToInterceptForObjectivesByClientId;

		void DeleteClientObjectives(const uint clientId, const uint missionId)
		{
			if (missionId > 0)
			{
				if (const auto& entry = objectiveByClientId.find(clientId); entry == objectiveByClientId.end() || entry->second.missionId != missionId)
					return;
			}
			objectiveByClientId.erase(clientId);
			objectivesByClientId.erase(clientId);
			if (bestPathToInterceptForObjectivesByClientId == clientId)
				bestPathToInterceptForObjectivesByClientId = 0;
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

				// Define target location
				XRequestBestPathEntry target;
				uint playerSystemId;
				pub::Player::GetSystem(clientId, playerSystemId);
				if (playerSystemId != objectiveEntry->second.systemId)
				{
					target = BestPath::GetJumpObjectToNextSystem(clientId, objectiveEntry->second.systemId);
				}
				else
				{
					target.systemId = objectiveEntry->second.systemId;
					target.position = objectiveEntry->second.position;
					target.objId = objectiveEntry->second.objId;
				}

				// Tell the server to compute the best path
				XRequestBestPath bestPath;
				bestPath.noPathFound = false;
				bestPath.repId = 0;
				bestPath.waypointCount = 2;
				bestPath.entries[0] = start;
				bestPath.entries[1] = target;

				// Set a flag to intercept the actual client package because we use it for getting the resolved path.
				bestPathToInterceptForObjectivesByClientId = clientId;
				Server.RequestBestPath(clientId, (uchar*)&bestPath, 12 + (bestPath.waypointCount * 20));
			}
			else
			{
				// Just for Display message stuff without waypoints
			}
		}

		bool __stdcall Send_FLPACKET_COMMON_REQUEST_BEST_PATH(uint clientId, const XRequestBestPath& data, int size)
		{
			if (bestPathToInterceptForObjectivesByClientId != clientId)
			{
				returncode = DEFAULT_RETURNCODE;
				return true;
			}

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			bestPathToInterceptForObjectivesByClientId = 0;

			// Early exit if no "best path" was generated or the client has somehow "left" by now.
			if (data.waypointCount == 0 || !HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId)) 
				return false;

			std::vector<pub::Player::MissionObjective> objs;
			objs.resize(data.waypointCount);

			int objectivesOffset = 0;
			IObjRW* inspect;
			StarSystem* starSystem;

			// If the player is docked, add first objective LAUNCH.
			uint baseId;
			pub::Player::GetBase(clientId, baseId);
			if (baseId > 0)
			{
				for (const auto& base : lstBases)
				{
					if (base.iBaseID == baseId && base.iObjectID)
					{
						objs.resize(data.waypointCount + 1);
						objs[0].type = pub::Player::MissionObjectiveType::MissionText;
						objs[0].message = FmtStr(13081, 0);
						objs[0].message.append_base(baseId);
						FmtStr::NavMarker marker;
						uint objId = base.iObjectID;
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
						objs[0].message.append_nav_marker(marker);
						objectivesOffset++;
						break;
					}
				}
			}

			// The first "best path" waypoint is being treated as objective to reach the actual destination.
			if (data.entries[0].objId > 0)
			{
				objs[objectivesOffset].type = pub::Player::MissionObjectiveType::ObjectiveWaypoint;
				IObjRW* inspect;
				StarSystem* system;
				uint objId = data.entries[0].objId;
				if (GetShipInspect(objId, inspect, system))
				{
					if (inspect->cobj->type & ObjectType::TradelaneRing)
					{
						if (data.entries[data.waypointCount - 1].objId > 0 && GetShipInspect(objId, inspect, system) && (inspect->cobj->type & ObjectType::JumpGate))
						{
							objs[objectivesOffset].message = FmtStr(13071, 0);
							objs[objectivesOffset].message.append_system(reinterpret_cast<CSolar*>(inspect->cobj)->jumpDestSystem);
						}
						else
						{
							objs[objectivesOffset].message = FmtStr(13060, 0);
						}
					}
					else if (inspect->cobj->type & ObjectType::JumpGate)
					{
						objs[objectivesOffset].message = FmtStr(13080, 0);
						objs[objectivesOffset].message.append_system(reinterpret_cast<CSolar*>(inspect->cobj)->jumpDestSystem);
					}
				}
			}

			// The final waypoint is the actual destination. Mark is as objective.
			objs[objs.size() - 1].type = pub::Player::MissionObjectiveType::ObjectiveWaypoint;

			// Translate all "best path" waypoints to nav map markers the objectives.
			for (int waypointIndex = 0, objectiveIndex = objectivesOffset; waypointIndex < data.waypointCount; waypointIndex++, objectiveIndex++)
			{
				FmtStr::NavMarker marker;
				marker.pos = data.entries[waypointIndex].position;
				marker.system = data.entries->systemId;
				objs[objectiveIndex].message.append_nav_marker(marker);
			}

			// Check whether the client needs a packet with new objectives. This must be done to ensure constant re-generation of best path will not flood the network.
			bool clientNeedsUpdate = false;
			if (!objectivesByClientId.contains(clientId) || objectivesByClientId[clientId].size() != objs.size())
			{
				clientNeedsUpdate = true;
			}
			else
			{
				for (int index = 0, length = objs.size(); index < length; index++)
				{
					if (objectivesByClientId[clientId][index].type != objs[index].type)
					{
						clientNeedsUpdate = true;
						break;
					}

					char flatFmtStrA[4096];
					const size_t sizeA = objectivesByClientId[clientId][index].message.flatten(flatFmtStrA, 4096);
					char flatFmtStrB[4096];
					const size_t sizeB = objs[index].message.flatten(flatFmtStrB, 4096);
					if (sizeA != sizeB || std::memcmp(flatFmtStrA, flatFmtStrB, sizeA) != 0)
					{
						clientNeedsUpdate = true;
						break;
					}
				}
			}

			// Save the current objectives for later comparison.
			objectivesByClientId[clientId] = objs;

			if (clientNeedsUpdate)
			{
				FmtStr missionTitle(13052, 0);
				FmtStr missionDescription(13052, 0);
				pub::Player::SetMissionObjectives(clientId, 12, objs.data(), objs.size(), missionTitle, 2, missionDescription);
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