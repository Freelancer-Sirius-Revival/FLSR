#include "Duel.h"
#include "DeathPenalty.h"
#include "IFF.h"
#include "Mark.h"
#include "Plugin.h"

namespace Duel
{
	const float MaxDistanceToLastBase = 10000.0f;
	const mstime DuelTimeOutMs = 30000;

	std::unordered_map<uint, std::pair<uint, Vector>> systemIdAndPositionByBaseId;
	std::unordered_map<uint, bool> outOfRangeFromLastBaseByClientId;

	void CacheDockableSolars()
	{
		CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
		while (solar != nullptr)
		{
			if ((solar->get_type() == ObjectType::Station || solar->get_type() == ObjectType::DockingRing) && solar->dockWithBaseId)
				systemIdAndPositionByBaseId.insert({ solar->dockWithBaseId, { solar->system, solar->get_position() } });
			solar = static_cast<CSolar*>(solar->FindNext());
		}
	}

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		ConPrint(L"Initializing Duels... ");

		CacheDockableSolars();

		ConPrint(L"Done\n");
	}

	static std::wstring GetCharacterName(const uint clientId)
	{
		return (wchar_t*)Players.GetActiveCharacterName(clientId);
	}

	static uint GetClientId(const std::wstring& characterName)
	{
		uint clientId = 0;
		bool idStringType = false;
		if (HkGetClientID(idStringType, clientId, characterName) == HKE_OK)
			return clientId;
		return 0;
	}

	static std::wstring GetCharacterNameByTarget(const uint targettingClientId)
	{
		uint shipId;
		pub::Player::GetShip(targettingClientId, shipId);
		if (!shipId)
			return L"";

		uint targetId;
		pub::SpaceObj::GetTarget(shipId, targetId);
		if (!targetId)
			return L"";

		const uint targetClientId = HkGetClientIDByShip(targetId);
		if (targetClientId)
			return GetCharacterName(targetClientId);

		return L"";
	}

	struct Duel
	{
		uint hostClientId = 0;
		uint guestClientId = 0;
		mstime invitationTime = 0;
		bool ongoing = false;
		std::pair<IFF::Attitude, IFF::Attitude> initialAttitude;
	};

	std::vector<Duel> duels;

	static bool IsPartOfDuel(const uint clientId, const Duel& duel)
	{
		return duel.hostClientId == clientId || duel.guestClientId == clientId;
	}

	static std::vector<Duel>::iterator FindDuel(const uint clientIdA, const uint clientIdB)
	{
		for (auto it = duels.begin(); it != duels.end(); it++)
			if (IsPartOfDuel(clientIdA, *it) && IsPartOfDuel(clientIdB, *it))
				return it;
		return duels.end();
	}

	static std::vector<Duel>::iterator RemoveDuel(const std::vector<Duel>::iterator duel)
	{
		uint shipIdA, shipIdB;
		pub::Player::GetShip(duel->hostClientId, shipIdA);
		pub::Player::GetShip(duel->guestClientId, shipIdB);
		if (shipIdA && shipIdB)
		{
			Mark::UnmarkObject(duel->hostClientId, shipIdB);
			Mark::UnmarkObject(duel->guestClientId, shipIdA);
		}
		DeathPenalty::RemovePvPExclusion(duel->hostClientId, duel->guestClientId);
		IFF::SetAttitude(duel->hostClientId, duel->guestClientId, duel->initialAttitude.first);
		IFF::SetAttitude(duel->guestClientId, duel->hostClientId, duel->initialAttitude.second);
		return duels.erase(duel);
	}

	static bool TryCancelDuel(const uint clientIdA, const uint clientIdB)
	{
		const auto& duel = FindDuel(clientIdA, clientIdB);
		if (duel == duels.end())
			return false;

		if (duel->ongoing)
			return false;

		PrintUserCmdText(clientIdA, L"Duel cancelled.");
		PrintUserCmdText(clientIdB, GetCharacterName(clientIdA) + L" cancelled the duel.");
		RemoveDuel(duel);
		return true;
	}

	static bool IsInNoPvpSystem(const uint clientId)
	{
		uint systemId;
		pub::Player::GetSystem(clientId, systemId);
		for (const auto& noPvPSystem : map_mapNoPVPSystems)
		{
			if (noPvPSystem.second == systemId)
				return true;
		}
		return false;
	}

	static void SendGo(const uint clientIdA, const uint clientIdB)
	{
		pub::Player::DisplayMissionMessage(clientIdA, FmtStr(524389, 0), MissionMessageType::MissionMessageType_Type1, false);
		pub::Player::DisplayMissionMessage(clientIdB, FmtStr(524389, 0), MissionMessageType::MissionMessageType_Type1, false);

		uint shipIdA, shipIdB;
		pub::Player::GetShip(clientIdA, shipIdA);
		pub::Player::GetShip(clientIdB, shipIdB);
		if (!shipIdA || !shipIdB)
			return;
		pub::SpaceObj::SendComm(shipIdA, shipIdB, CreateID("announcer"), nullptr, 0, std::vector<uint>({ CreateID("DX_M06_0220_ANNOUNCER") }).data(), 1, 0, 0.5f, false);
		pub::SpaceObj::SendComm(shipIdB, shipIdA, CreateID("announcer"), nullptr, 0, std::vector<uint>({ CreateID("DX_M06_0220_ANNOUNCER") }).data(), 1, 0, 0.5f, false);
	}

	static float GetPlayerDistanceToLastBase(const uint clientId)
	{
		const uint baseId = Players[clientId].iLastBaseID;
		if (!baseId)
			return -1.0f;

		if (const auto& entry = systemIdAndPositionByBaseId.find(baseId); entry != systemIdAndPositionByBaseId.end())
		{
			uint systemId = 0;
			pub::Player::GetSystem(clientId, systemId);
			if (systemId != entry->second.first)
				return -1.0f;

			uint shipId = 0;
			pub::Player::GetShip(clientId, shipId);
			IObjRW* inspect;
			StarSystem* system;
			if (shipId && GetShipInspect(shipId, inspect, system))
				return HkDistance3D(entry->second.second, inspect->get_position());
		}

		return -1.0f;
	}

	static bool MovedTooFarAwayFromLastBase(const uint clientId)
	{
		const auto& entry = outOfRangeFromLastBaseByClientId.find(clientId);
		if (entry == outOfRangeFromLastBaseByClientId.end())
		{
			const bool outOfRange = GetPlayerDistanceToLastBase(clientId) > MaxDistanceToLastBase;
			outOfRangeFromLastBaseByClientId.insert({ clientId, outOfRange });
			return outOfRange;
		}
		else if (!entry->second)
			entry->second = GetPlayerDistanceToLastBase(clientId) > MaxDistanceToLastBase;
		return entry->second;
	}

	bool UserCmds(const uint clientId, const std::wstring& arguments)
	{
		if (ToLower(arguments).find(L"/duel") == 0)
		{
			std::wstring targetCharacterName = Trim(GetParamToEnd(arguments, ' ', 1));
			if (targetCharacterName.empty())
				targetCharacterName = GetCharacterNameByTarget(clientId);
			const uint targetClientId = GetClientId(targetCharacterName);

			if (targetClientId == clientId)
			{
				PrintUserCmdText(clientId, L"Select someone else to duel with.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			if (!HkIsValidClientID(targetClientId))
			{
				PrintUserCmdText(clientId, L"You must select a duel partner. Either click on the ship, or type /duel <player name>.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			const auto& duel = FindDuel(clientId, targetClientId);
			if (duel != duels.end())
			{
				if (duel->ongoing)
				{
					PrintUserCmdText(clientId, L"You are already in a duel with " + targetCharacterName + L".");
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return true;
				}
				else if (duel->hostClientId == clientId)
				{
					PrintUserCmdText(duel->hostClientId, L"Duel cancelled.");
					PrintUserCmdText(duel->guestClientId, GetCharacterName(duel->hostClientId) + L" cancelled the duel.");
					RemoveDuel(duel);

					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return true;
				}
			}

			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			if (!shipId)
			{
				PrintUserCmdText(clientId, L"You must be in space to invite to a duel.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			if (IsInNoPvpSystem(clientId))
			{
				PrintUserCmdText(clientId, L"PvP is disabled in this system. Duels are not possible.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			uint missionId;
			pub::Player::GetMsnID(clientId, missionId);
			if (missionId)
			{
				PrintUserCmdText(clientId, L"You must not be in a mission for a duel.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			if (MovedTooFarAwayFromLastBase(clientId))
			{
				PrintUserCmdText(clientId, L"You moved more than " + std::to_wstring((int)(MaxDistanceToLastBase / 1000.0f)) + L"K away from your last docked base. Re-dock with it or another base to duel.");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			pub::Player::GetShip(targetClientId, shipId);
			if (!shipId)
			{
				PrintUserCmdText(clientId, L"Your chosen partner cannot accept duels right now.");
				PrintUserCmdText(targetClientId, GetCharacterName(clientId) + L" wishes to duel with you. But you must be in space.");

				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			pub::Player::GetMsnID(targetClientId, missionId);
			if (missionId)
			{
				PrintUserCmdText(clientId, L"Your chosen partner cannot accept duels right now.");
				PrintUserCmdText(targetClientId, GetCharacterName(clientId) + L" wishes to duel with you. But you are still in a mission.");

				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			if (MovedTooFarAwayFromLastBase(targetClientId))
			{
				PrintUserCmdText(clientId, L"Your chosen partner cannot accept duels right now.");
				PrintUserCmdText(targetClientId, GetCharacterName(clientId) + L" wishes to duel with you. But you were already more than " + std::to_wstring((int)(MaxDistanceToLastBase / 1000.0f)) + L"K away from your last docked base. Re-dock with it or another base.");
				
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return true;
			}

			if (duel != duels.end())
			{
				if (!duel->ongoing && clientId != duel->hostClientId)
				{
					duel->ongoing = true;
					duel->initialAttitude = IFF::GetAttitudeTowards({ GetCharacterName(clientId), targetCharacterName });
					DeathPenalty::AddPvPExclusion(duel->hostClientId, duel->guestClientId);
					PrintUserCmdText(duel->hostClientId, L"Duel accepted with " + GetCharacterName(duel->guestClientId) + L". Start!");
					PrintUserCmdText(duel->guestClientId, L"Duel accepted with " + GetCharacterName(duel->hostClientId) + L". Start!");
					SendGo(duel->hostClientId, duel->guestClientId);

					IFF::SetAttitude(duel->hostClientId, duel->guestClientId, IFF::Attitude::Hostile);
					IFF::SetAttitude(duel->guestClientId, duel->hostClientId, IFF::Attitude::Hostile);

					uint shipIdA, shipIdB;
					pub::Player::GetShip(duel->hostClientId, shipIdA);
					pub::Player::GetShip(duel->guestClientId, shipIdB);
					if (shipIdA && shipIdB)
					{
						Mark::MarkObject(duel->hostClientId, shipIdB);
						Mark::MarkObject(duel->guestClientId, shipIdA);
					}

					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return true;
				}
			}

			Duel newDuel;
			newDuel.hostClientId = clientId;
			newDuel.guestClientId = targetClientId;
			newDuel.invitationTime = timeInMS();
			newDuel.ongoing = false;
			duels.push_back(newDuel);
			PrintUserCmdText(clientId, L"Duel invitation sent to " + GetCharacterName(targetClientId) + L".");
			PrintUserCmdText(targetClientId, GetCharacterName(clientId) + L" wishes to duel with you. Select their ship or type /duel " + GetCharacterName(clientId) + L" to accept.");
			
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		returncode = DEFAULT_RETURNCODE;
		return false;
	}

	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		if (killedObject->is_player())
		{
			const uint clientId = killedObject->cobj->ownerPlayer;
			outOfRangeFromLastBaseByClientId.erase(clientId);
			for (auto it = duels.begin(); it < duels.end();)
			{
				if (IsPartOfDuel(clientId, *it))
				{
					if (it->ongoing)
					{
						PrintUserCmdText(it->hostClientId, L"Duel with " + GetCharacterName(it->guestClientId) + L" ended.");
						PrintUserCmdText(it->guestClientId, L"Duel with " + GetCharacterName(it->hostClientId) + L" ended.");
					}
					else
					{
						PrintUserCmdText(it->guestClientId, L"Duel invitation of " + GetCharacterName(it->hostClientId) + L" aborted.");
					}
					it = RemoveDuel(it);
				}
				else
					it++;
			}
		}
	}

	float elapsedTimeInSec = 0.0f;
	void __stdcall Elapse_Time_AFTER(float seconds)
	{
		elapsedTimeInSec += seconds;
		if (elapsedTimeInSec < 1.0f)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}
		elapsedTimeInSec = 0.0f;

		PlayerData* playerData = nullptr;
		while (playerData = Players.traverse_active(playerData))
		{
			if (playerData->iShipID)
				MovedTooFarAwayFromLastBase(playerData->iOnlineID);
		}

		const mstime now = timeInMS();
		for (auto it = duels.begin(); it < duels.end();)
		{
			if (it->ongoing)
			{
				if (MovedTooFarAwayFromLastBase(it->hostClientId))
				{
					PrintUserCmdText(it->hostClientId, L"Duel ended, because you are too far away from your last base.");
					PrintUserCmdText(it->guestClientId, L"Duel with " + GetCharacterName(it->hostClientId) + L" ended, because they are too far away from their last base.");
					DeathPenalty::RemovePvPExclusion(it->hostClientId, it->guestClientId);
					it = RemoveDuel(it);
				}
				else if (MovedTooFarAwayFromLastBase(it->guestClientId))
				{
					PrintUserCmdText(it->guestClientId, L"Duel ended, because you are too far away from your last base.");
					PrintUserCmdText(it->hostClientId, L"Duel with " + GetCharacterName(it->guestClientId) + L" ended, because they are too far away from their last base.");
					DeathPenalty::RemovePvPExclusion(it->hostClientId, it->guestClientId);
					it = RemoveDuel(it);
				}
				else
					it++;
			}
			else if (now - it->invitationTime > DuelTimeOutMs)
			{
				PrintUserCmdText(it->hostClientId, L"Duel invitation with " + GetCharacterName(it->guestClientId) + L" timed out.");
				PrintUserCmdText(it->guestClientId, L"Duel invitation with " + GetCharacterName(it->hostClientId) + L" timed out.");
				it = RemoveDuel(it);
			}
			else
				it++;
		}

		returncode = DEFAULT_RETURNCODE;
	}

	std::unordered_set<uint> clientUndockedFromBase;

	void __stdcall DisConnect_After(unsigned int clientId, enum EFLConnection p2)
	{
		clientUndockedFromBase.erase(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall CharacterSelect_After(const CHARACTER_ID& cId, unsigned int clientId)
	{
		clientUndockedFromBase.erase(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseExit_After(unsigned int baseId, unsigned int clientId)
	{
		clientUndockedFromBase.insert(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
	{
		if (!clientUndockedFromBase.contains(clientId))
			outOfRangeFromLastBaseByClientId.insert({ clientId, true });
		clientUndockedFromBase.erase(clientId);
		returncode = DEFAULT_RETURNCODE;
	}
}