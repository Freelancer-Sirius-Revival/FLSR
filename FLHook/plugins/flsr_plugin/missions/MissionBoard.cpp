#include "../Main.h"
#include "MissionBoard.h"
#include "Missions.h"
#include "randomMissions/Meta.h"

namespace MissionBoard
{
	struct MissionPacketFirstPart
	{
		uint missionId;
		uint base;
		uint index;
		uint unk = 2;
		uint type;
	};

	static void SendOfferToClient(const uint clientId, const uint offerId, const Offer& offer, const uint base, const uint index)
	{
		char* buffer = (char*)std::malloc(1024);
		MissionPacketFirstPart* data = (MissionPacketFirstPart*)buffer;
		data->missionId = offerId;
		data->base = base;
		data->index = index;
		data->unk = 2; //Static value that is always present
		data->type = (uint)offer.type;
		size_t pos = sizeof(MissionPacketFirstPart);
		FmtStr system(1844, 0);
		system.append_system(offer.system);
		pos += system.flatten(buffer + pos, 0);
		FmtStr faction(1834, 0);
		uint groupName;
		pub::Reputation::GetGroupName(offer.group, groupName);
		faction.append_string(groupName);
		pos += faction.flatten(buffer + pos, 0);
		FmtStr message(327681, 0);
		FmtStr header(327682, 0);
		header.append_int(offer.reward);
		message.append_fmt_str(header);
		message.append_fmt_str(offer.description);
		pos += message.flatten(buffer + pos, 0);
		std::memcpy(buffer + pos, &offer.reward, sizeof(uint));
		pos += sizeof(uint);
		GetClientInterface()->Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER(clientId, buffer, pos);
	}

	struct MissionAcceptance
	{
		uint index;
		uint base;
		// acceptanceData:
		// 1 byte:  bool accepted
		// 2 bytes: ushort rejectedResourceId;
		// 4 bytes: unknown
		char acceptanceData[7];
	};

	static void SendOfferAcceptance(const uint clientId, const uint boardIndex, const uint base)
	{
		MissionAcceptance data;
		data.index = boardIndex;
		data.base = base;
		std::memset(&data.acceptanceData, 0, sizeof(data.acceptanceData));
		data.acceptanceData[0] = 1; // TRUE, accepted
		GetClientInterface()->Send_FLPACKET_SERVER_GFMISSIONVENDORACCEPTANCE(clientId, &data, sizeof(MissionAcceptance));
	}

	static void SendOfferRejection(const uint clientId, const uint boardIndex, const uint base, const ushort rejectionResourceId)
	{
		MissionAcceptance data;
		data.index = boardIndex;
		data.base = base;
		std::memset(&data.acceptanceData, 0, sizeof(data.acceptanceData));
		data.acceptanceData[0] = 0; // FALSE, rejected
		ushort* rejectedText = (ushort*)& data.acceptanceData[1];
		*rejectedText = rejectionResourceId;
		GetClientInterface()->Send_FLPACKET_SERVER_GFMISSIONVENDORACCEPTANCE(clientId, &data, sizeof(MissionAcceptance));
	}

	std::unordered_map<uint, Offer> offers;
	std::unordered_map<uint, std::unordered_set<uint>> offerIdsByBaseId; // List of public offers
	std::unordered_map<uint, std::unordered_set<uint>> offerIdsByClientId; // List of private offers
	struct BoardEntry
	{
		uint boardIndex;
		uint offerId;
	};
	std::unordered_map<uint, std::vector<BoardEntry>> boardIndicesByClientId;

	static void SendDestroyOfferToAll(const uint offerId)
	{
		for (const auto& clientEntry : boardIndicesByClientId)
		{
			for (auto indexEntry = clientEntry.second.begin(); indexEntry != clientEntry.second.end(); indexEntry++)
			{
				if (indexEntry->offerId == offerId)
				{
					const uint clientId = clientEntry.first;
					uint base = 0;
					pub::Player::GetBase(clientId, base);
					if (base)
						GetClientInterface()->Send_FLPACKET_SERVER_GFDESTROYMISSIONCOMPUTER(clientId, base, offerId);
					// Do not clear boardIndicesByClientId here. Other clients may still have the offer visible if they have the job board open at the same time.
					break;
				}
			}
		}
	}

	uint offerId = UINT_MAX;

	uint AddPublicOffer(const Offer& offer, const std::unordered_set<uint>& baseIds)
	{
		offers.insert({ offerId, offer });
		for (const uint baseId : baseIds)
			offerIdsByBaseId[baseId].insert(offerId);
		// To avoid collisions with Freelancer's own mission offers. Count the custom mission offer IDs from high to low.
		return offerId--;
	}

	uint AddPrivateOffer(const Offer& offer, const uint clientId)
	{
		offers.insert({ offerId, offer });
		offerIdsByClientId[clientId].insert(offerId);
		// To avoid collisions with Freelancer's own mission offers. Count the custom mission offer IDs from high to low.
		return offerId--;
	}

	void DeleteOffer(const uint offerId)
	{
		offers.erase(offerId);
		for (auto& entry : offerIdsByClientId)
			entry.second.erase(offerId);
		for (auto& entry : offerIdsByBaseId)
			entry.second.erase(offerId);
		SendDestroyOfferToAll(offerId);
	}

	void __stdcall MissionResponse(uint boardIndex, uint origin, bool accepted, uint clientId)
	{
		if (!accepted)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		st6::vector<uint> groupMembers;
		pub::Player::GetGroupMembers(clientId, groupMembers);
		// Usually this check is enough on one player of a group. But just make sure it really does not overlook anything.
		for (const auto memberId : groupMembers)
		{
			uint missionId;
			pub::Player::GetMsnID(clientId, missionId);
			if (missionId > 0 || Missions::IsPartOfOfferedJob(clientId))
			{
				uint base;
				pub::Player::GetBase(clientId, base);
				SendOfferRejection(clientId, boardIndex, base, 1840);
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return;
			}
		}

		if (const auto& clientEntry = boardIndicesByClientId.find(clientId); clientEntry != boardIndicesByClientId.end())
		{
			for (const auto& boardEntry : clientEntry->second)
			{
				if (boardEntry.boardIndex == boardIndex)
				{
					uint base = 0;
					pub::Player::GetBase(clientId, base);
					if (!base)
					{
						returncode = SKIPPLUGINS_NOFUNCTIONCALL;
						return;
					}

					// Nobody has yet taken upon the offer
					if (const auto& offerEntry = offers.find(boardEntry.offerId); offerEntry != offers.end())
					{
						SendOfferAcceptance(clientId, boardIndex, base);
						std::vector<uint> groupMemberIds;
						for (const auto memberId : groupMembers)
						{
							// SetMsnID makes sure: 1. the player cannot be invited to groups ("Is in a Mission") and 2. it sets the same MsnId to late group-joining players.
							pub::Player::SetMsnID(memberId, boardEntry.offerId, clientId, false, 0);
							groupMemberIds.push_back(memberId);
						}
						Missions::StartMissionByOfferId(boardEntry.offerId, clientId, groupMemberIds);
						DeleteOffer(boardEntry.offerId);
					}
					// Someone already removed it from the pool
					else
					{
						SendOfferRejection(clientId, boardIndex, base, 524389); // From FLSR Dialogs resources
					}

					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return;
				}
			}
		}
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall AbortMission(uint clientId, uint missionId)
	{
		if (Missions::IsPartOfOfferedJob(clientId))
		{
			Missions::RemoveClientFromCurrentOfferedJob(clientId);
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return;
		}
		returncode = DEFAULT_RETURNCODE;
	}

	static HMODULE LoadContentDll()
	{
		INI_Reader ini;
		if (ini.open("freelancer.ini", false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Initial MP DLLs"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("path"))
						{
							const std::string dataPath = ini.get_value_string(0);
							ini.close();
							return GetModuleHandle((dataPath + "\\Content.dll").c_str());
						}
					}
					break;
				}
			}
			ini.close();
		}
		return 0;
	}

	std::unordered_map<uint, uint> boardLastIndexByClient;

	bool __stdcall Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER(uint clientId, void* data, uint dataSize)
	{
		const uint index = *((uint*)data + 2);
		boardLastIndexByClient[clientId] = std::max<uint>(boardLastIndexByClient[clientId], index);
		returncode = DEFAULT_RETURNCODE;
		return true;
	}

	bool __stdcall Send_FLPACKET_SERVER_GFCOMPLETEMISSIONCOMPUTERLIST(uint clientId, uint base)
	{
		// Before the Complete Packet it sent, add the custom missions to the list.

		uint shipArchetypeId = 0;
		pub::Player::GetShipID(clientId, shipArchetypeId);

		int clientRep = 0;
		pub::Player::GetRep(clientId, clientRep);

		// Add all private offers for the player
		std::unordered_set<uint> offerIds(offerIdsByClientId[clientId]);
		// And now add all public offers for the current base.
		if (const auto& entry = offerIdsByBaseId.find(base); entry != offerIdsByBaseId.end())
			offerIds.insert(entry->second.begin(), entry->second.end());

		for (const auto offerId : offerIds)
		{
			const auto& offer = offers.at(offerId);
			float feelings;
			pub::Reputation::GetGroupFeelingsTowards(clientRep, offer.group, feelings);
			if (feelings <= RandomMissions::minRequiredReputationForMissions)
				continue;
			if (!offer.allowedShipArchetypeIds.empty() && !offer.allowedShipArchetypeIds.contains(shipArchetypeId))
				continue;

			const uint index = ++boardLastIndexByClient[clientId];
			boardIndicesByClientId[clientId].push_back({ index, offerId });
			SendOfferToClient(clientId, offerId, offer, base, index);
		}

		returncode = DEFAULT_RETURNCODE;
		return true;
	}

	static void ClearJobBoardClientData(const uint clientId)
	{
		boardIndicesByClientId.erase(clientId);
		boardLastIndexByClient.erase(clientId);
	}

	void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
	{
		ClearJobBoardClientData(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		ClearJobBoardClientData(clientId);
		returncode = DEFAULT_RETURNCODE;
	}
}