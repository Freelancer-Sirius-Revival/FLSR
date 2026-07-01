#include "SolarDocking.h"
#include "../Plugin.h"
#include <random>

namespace Missions
{
	std::random_device rd;
	std::mt19937 gen(rd());

	struct SpawnedSolar
	{
		uint dockWith = 0;
	};
	std::unordered_map<uint, SpawnedSolar> dockableSolars;

	void RegisterDockableSolar(const uint objId, const uint baseId)
	{
		dockableSolars.insert({ objId, { baseId } });
	}

	namespace Hooks
	{
		namespace SolarDocking
		{
			struct LaunchComm
			{
				uint solarObjId;
				uint dockId;
			};

			std::map<uint, LaunchComm> unprocessedLaunchComms;

			bool __stdcall Send_FLPACKET_SERVER_LAUNCH(uint iClientID, FLPACKET_LAUNCH& pLaunch)
			{
				if (dockableSolars.contains(pLaunch.iSolarObjId))
				{
					LaunchComm comm;
					comm.solarObjId = pLaunch.iSolarObjId;
					comm.dockId = pLaunch.iDock;
					unprocessedLaunchComms[iClientID] = comm;
				}
				returncode = DEFAULT_RETURNCODE;
				return true;
			}

			void __stdcall PlayerLaunch(unsigned int objId, unsigned int clientId)
			{
				// In here the player will be moved to open space above the center of the system in case there is no existing dock for the base.
				// Otherwise the server will crash because the player is tried to be undocking from a non-existing dock.
				auto& playerData = Players[clientId];
				for (const auto& base : lstBases)
				{
					if (base.iBaseID == playerData.exitedBase && pub::SpaceObj::ExistsAndAlive(base.iObjectID) != 0) // 0 -> true
					{
						bool solarFound = false;
						for (const auto& dockable : dockableSolars)
						{
							if (dockable.second.dockWith == playerData.exitedBase)
							{
								solarFound = true;
								break;
							}
						}
						if (!solarFound)
						{
							playerData.exitedBase = 0;
							playerData.vPosition.y = 20000.0f;
						}
						break;
					}
				}
				returncode = DEFAULT_RETURNCODE;
			}

			static uint GetShipMessageId(const uint shipId)
			{
				IObjRW* inspect;
				StarSystem* starSystem;
				if (!GetShipInspect(shipId, inspect, starSystem))
					return 0;
				const Archetype::Ship* shipArchetype = static_cast<Archetype::Ship*>(inspect->cobj->archetype);
				char msgIdPrefix[64];
				strncpy_s(msgIdPrefix, sizeof(msgIdPrefix), shipArchetype->msgidprefix_str, shipArchetype->msgidprefix_len);
				return CreateID(msgIdPrefix);
			}

			static bool SendLaunchWellWishes(const uint shipId, const uint solarObjId, const uint dockId)
			{
				IObjRW* inspect;
				StarSystem* starSystem;
				if (!GetShipInspect(solarObjId, inspect, starSystem))
					return false;
				const CEqObj* solar = static_cast<CEqObj*>(inspect->cobj);
				if (!solar->voiceId)
					return false;
				const Archetype::EqObj* solarArchetype = static_cast<Archetype::EqObj*>(solar->archetype);
				Archetype::DockType dockType;
				try
				{
					dockType = solarArchetype->dockInfo.at(dockId).dockType;
				}
				catch (const std::out_of_range& e)
				{
					return false;
				}

				std::string clearMessageIdBase;
				switch (dockType)
				{
				case Archetype::DockType::Berth:
					clearMessageIdBase = "gcs_docklaunch_clear_berth_0";
					break;

				case Archetype::DockType::MoorSmall:
				case Archetype::DockType::MoorMedium:
				case Archetype::DockType::MoorLarge:
					clearMessageIdBase = "gcs_docklaunch_clear_moor_0";
					break;

				case Archetype::DockType::Ring:
					clearMessageIdBase = "gcs_docklaunch_clear_ring_0";
					break;

				default:
					return false;
				}

				std::uniform_int_distribution<> distr(1, 2);
				std::vector<uint> lines = {
					GetShipMessageId(shipId),
					CreateID((clearMessageIdBase + std::to_string(distr(gen)) + "-").c_str()),
					CreateID(("gcs_misc_wellwish_0" + std::to_string(distr(gen)) + "-").c_str())
				};
				pub::SpaceObj::SendComm(solar->id, shipId, solar->voiceId, &solar->commCostume, 0, lines.data(), lines.size(), 19007 /* base comms type*/, 0.5f, false);
				return true;
			}

			void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
			{
				if (!unprocessedLaunchComms.contains(clientId))
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}

				SendLaunchWellWishes(shipId, unprocessedLaunchComms[clientId].solarObjId, unprocessedLaunchComms[clientId].dockId);
				unprocessedLaunchComms.erase(clientId);
				returncode = DEFAULT_RETURNCODE;
			}

			struct DockQueue
			{
				bool waiting;
			};

			std::unordered_map<uint, std::unordered_map<uint, DockQueue>> dockQueues;

			// Gets called whenever a dock request begins, ends, is cancelled, or the ship is destroyed/despawned. Does not get called when the station gets destroyed.
			int __cdecl Dock_Call_After(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, DOCK_HOST_RESPONSE response)
			{
				if (!dockableSolars.contains(dockTargetId))
				{
					returncode = DEFAULT_RETURNCODE;
					return 0;
				}

				// dockPortIndex -1 means docking was cancelled.
				if (dockPortIndex < 0 || response == DOCK_HOST_RESPONSE::DOCK)
				{
					dockQueues[dockTargetId].erase(ship);
					returncode = DEFAULT_RETURNCODE;
					return 0;
				}

				dockQueues[dockTargetId][ship].waiting = response == DOCK_HOST_RESPONSE::DOCK_IN_USE;

				IObjRW* inspect;
				StarSystem* starSystem;
				if (!GetShipInspect(dockTargetId, inspect, starSystem))
				{
					returncode = DEFAULT_RETURNCODE;
					return 0;
				}
				const CEqObj* solar = static_cast<CEqObj*>(inspect->cobj);
				if (!solar->voiceId)
				{
					returncode = DEFAULT_RETURNCODE;
					return 0;
				}
				const Archetype::EqObj* solarArchetype = static_cast<Archetype::EqObj*>(solar->archetype);
				Archetype::DockType dockType;
				try
				{
					dockType = solarArchetype->dockInfo.at(dockPortIndex).dockType;
				}
				catch (const std::out_of_range& e)
				{
					returncode = DEFAULT_RETURNCODE;
					return 0;
				}

				std::vector<uint> lines;
				switch (response)
				{
				case DOCK_HOST_RESPONSE::PROCEED_DOCK:
				{
					std::string dockTypeMessageId;
					switch (dockType)
					{
					case Archetype::DockType::Berth:
						dockTypeMessageId = "gcs_dockrequest_todock";
						break;

					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockTypeMessageId = "gcs_dockrequest_tomoor";
						break;

					case Archetype::DockType::Ring:
						dockTypeMessageId = "gcs_dockrequest_toland";
						break;

					default:
						dockTypeMessageId = "";
						break;
					}

					std::string dockTargetMessageId;
					switch (dockType)
					{
					case Archetype::DockType::Berth:
						dockTargetMessageId = "gcs_dockrequest_todock_number";
						break;

					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockTargetMessageId = "gcs_dockrequest_tomoor_number";
						break;

					case Archetype::DockType::Ring:
						dockTargetMessageId = "gcs_dockrequest_toland-";
						break;

					default:
						dockTargetMessageId = "";
						break;
					}


					std::string dockNumberMessageId;
					switch (dockType)
					{
					case Archetype::DockType::Berth:
					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockNumberMessageId = "gcs_misc_number_" + std::to_string(dockPortIndex + 1) + "-";
						break;

					default:
						dockNumberMessageId = "";
						break;
					}

					if (dockQueues[dockTargetId][ship].waiting)
					{
						lines = {
							GetShipMessageId(ship),
							CreateID("gcs_dockrequest_nowcleared_01+"),
							CreateID((!dockTypeMessageId.empty() ? (dockTypeMessageId + "-") : "").c_str()),
							CreateID("gcs_dockrequest_proceed_01+"),
							CreateID(dockTargetMessageId.c_str()),
							CreateID(dockNumberMessageId.c_str())
						};
					}
					else
					{
						lines = {
							CreateID(("gcs_misc_ack_0" + std::to_string(std::uniform_int_distribution(1, 3)(gen)) + "-").c_str()),
							CreateID("gcs_dockrequest_yourrequest+"),
							CreateID(dockTypeMessageId.c_str()),
							CreateID("gcs_dockrequest_granted_01-"),
							CreateID("gcs_dockrequest_proceed_01+"),
							CreateID(dockTargetMessageId.c_str()),
							CreateID(dockNumberMessageId.c_str())
						};
					}
					break;
				}



				case DOCK_HOST_RESPONSE::DOCK_IN_USE:
				{
					lines = {
						CreateID(("gcs_misc_ack_0" + std::to_string(std::uniform_int_distribution(1, 3)(gen)) + "-").c_str()),
						CreateID("gcs_dockrequest_standby_01-"),
						CreateID("gcs_dockrequest_delayedreason_01-"),
						CreateID("gcs_dockrequest_willbecleared_01-")
					};
					break;
				}



				case DOCK_HOST_RESPONSE::DOCK_DENIED:
				{
					std::string dockTypeMessageId;
					switch (dockType)
					{
					case Archetype::DockType::Berth:
						dockTypeMessageId = "gcs_dockrequest_todock";
						break;

					case Archetype::DockType::MoorSmall:
					case Archetype::DockType::MoorMedium:
					case Archetype::DockType::MoorLarge:
						dockTypeMessageId = "gcs_dockrequest_tomoor";
						break;

					case Archetype::DockType::Ring:
						dockTypeMessageId = "gcs_dockrequest_toland";
						break;

					default:
						dockTypeMessageId = "";
						break;
					}

					lines = {
						CreateID(("gcs_misc_ack_0" + std::to_string(std::uniform_int_distribution(1, 3)(gen)) + "-").c_str()),
						CreateID("gcs_dockrequest_yourrequest+"),
						CreateID(dockTypeMessageId.c_str()),
						CreateID("gcs_dockrequest_denied_01-"),
						CreateID("gcs_dockrequest_nofit_01-")
					};
					break;
				}



				case DOCK_HOST_RESPONSE::ACCESS_DENIED:
					lines = { CreateID("gcs_dockrequest_denied_01-") };
					break;
				}

				uint shipId = ship;
				pub::SpaceObj::SendComm(solar->id, shipId, solar->voiceId, &solar->commCostume, 0, lines.data(), lines.size(), 19007 /* base comms type*/, 0.5f, true);
				returncode = DEFAULT_RETURNCODE;
				return 0;
			}

			void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
			{
				const uint objId = killedObject->cobj->id;
				if (!dockableSolars.contains(objId))
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}
				dockableSolars.erase(objId);

				IObjRW* inspect;
				StarSystem* starSystem;
				for (const auto& entry : dockQueues[objId])
				{
					uint shipId = entry.first;
					// Check for invulnerability to know if the player is in the dock scene without ship controls.
					if (GetShipInspect(shipId, inspect, starSystem) && inspect->cobj->ownerPlayer > 0 && inspect->is_invulnerable())
						pub::Player::ForceLand(inspect->cobj->ownerPlayer, static_cast<CEqObj*>(killedObject->cobj)->dockWithBaseId);
				}
				dockQueues.erase(objId);
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}