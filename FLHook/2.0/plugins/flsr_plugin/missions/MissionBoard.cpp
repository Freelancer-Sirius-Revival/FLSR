#include "../Main.h"
#include "MissionBoard.h"
#include "Missions.h"

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

	static void SendMissionOfferToClient(const uint clientId, const uint offerId, const MissionOffer& mission, const uint base, const uint index)
	{
		char* buffer = (char*)std::malloc(1024);
		MissionPacketFirstPart* data = (MissionPacketFirstPart*)buffer;
		data->missionId = offerId;
		data->base = base;
		data->index = index;
		data->unk = 2; //Static value that is always present
		data->type = (uint)mission.type;
		size_t pos = sizeof(MissionPacketFirstPart);
		FmtStr system(1844, 0);
		system.append_system(mission.system);
		pos += system.flatten(buffer + pos, 0);
		FmtStr faction(1834, 0);
		uint groupName;
		pub::Reputation::GetGroupName(mission.group, groupName);
		faction.append_string(groupName);
		pos += faction.flatten(buffer + pos, 0);
		FmtStr message(327681, 0);
		FmtStr header(327682, 0);
		header.append_int(mission.reward);
		message.append_fmt_str(header);
		message.append_fmt_str(FmtStr(mission.description, 0));
		pos += message.flatten(buffer + pos, 0);
		std::memcpy(buffer + pos, &mission.reward, sizeof(uint));
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

	static void SendMissionOfferAcceptance(const uint clientId, const uint boardIndex, const uint base)
	{
		MissionAcceptance data;
		data.index = boardIndex;
		data.base = base;
		std::memset(&data.acceptanceData, 0, sizeof(data.acceptanceData));
		data.acceptanceData[0] = 1; // TRUE, accepted
		GetClientInterface()->Send_FLPACKET_SERVER_GFMISSIONVENDORACCEPTANCE(clientId, &data, sizeof(MissionAcceptance));
	}

	static void SendMissionOfferRejection(const uint clientId, const uint boardIndex, const uint base, const ushort rejectionResourceId)
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

	std::unordered_map<uint, MissionOffer> offersByOfferId;
	std::unordered_map<uint, std::unordered_set<uint>> offerIdsByBase;
	std::unordered_map<uint, std::vector<std::pair<uint, uint>>> offerIndicesByClient;

	static void SendDestroyMissionOfferToAll(const uint offerId)
	{
		for (const auto& clientEntry : offerIndicesByClient)
		{
			for (auto indexEntry = clientEntry.second.begin(); indexEntry != clientEntry.second.end(); indexEntry++)
			{
				if (indexEntry->second == offerId)
				{
					const uint clientId = clientEntry.first;
					uint base;
					pub::Player::GetBase(clientId, base);
					if (base)
						GetClientInterface()->Send_FLPACKET_SERVER_GFDESTROYMISSIONCOMPUTER(clientId, base, offerId);
					// Do not clear offerIndicesByClient here. Other clients may still have the offer visible if they have the job board open at the same time.
					break;
				}
			}
		}
	}

	uint offerId = UINT_MAX;

	uint AddMissionOffer(const MissionOffer& mission, const std::vector<uint>& bases)
	{
		offersByOfferId.insert({ offerId, mission });
		for (const uint base : bases)
			offerIdsByBase[base].insert(offerId);
		// To avoid collisions with Freelancer's own mission offers. Count the custom mission offer IDs from high to low.
		return offerId--;
	}

	void DeleteMissionOffer(const uint offerId)
	{
		offersByOfferId.erase(offerId);
		for (auto& baseEntry : offerIdsByBase)
			baseEntry.second.erase(offerId);
		SendDestroyMissionOfferToAll(offerId);
	}

	void __stdcall MissionResponse(uint boardIndex, uint origin, bool accepted, uint clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (!accepted)
			return;

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
				SendMissionOfferRejection(clientId, boardIndex, base, 1840);
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return;
			}
		}

		const auto& clientEntry = offerIndicesByClient.find(clientId);
		if (clientEntry != offerIndicesByClient.end())
		{
			for (const auto& indexEntry : clientEntry->second)
			{
				if (indexEntry.first == boardIndex)
				{
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					uint base;
					pub::Player::GetBase(clientId, base);
					if (!base)
						return;
					// Nobody has yet taken upon the offer
					if (offersByOfferId.contains(indexEntry.second))
					{
						SendMissionOfferAcceptance(clientId, boardIndex, base);
						std::vector<uint> groupMemberIds;
						for (const auto memberId : groupMembers)
						{
							// SetMsnID makes sure: 1. the player cannot be invited to groups ("Is in a Mission") and 2. it sets the same MsnId to late group-joining players.
							pub::Player::SetMsnID(memberId, indexEntry.second, clientId, false, 0);
							groupMemberIds.push_back(memberId);
						}
						Missions::StartMissionByOfferId(indexEntry.second, groupMemberIds);
						DeleteMissionOffer(indexEntry.second);
					}
					// Someone already removed it from the pool
					else
					{
						SendMissionOfferRejection(clientId, boardIndex, base, 976);
					}
					return;
				}
			}
		}
	}

	void __stdcall AbortMission(uint clientId, uint missionId)
	{
		returncode = DEFAULT_RETURNCODE;
		if (Missions::IsPartOfOfferedJob(clientId))
		{
			Missions::RemoveClientFromCurrentOfferedJob(clientId);
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		}
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

	float minRequiredReputationForMissions = -0.2f;

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;
		
		const HMODULE contentHandle = LoadContentDll();
		if (contentHandle)
			minRequiredReputationForMissions = *(float*)(DWORD(contentHandle) + 0x1195BC);
	}

	std::unordered_map<uint, uint> missionBoardOffersMaxIndexByClient;

	bool __stdcall Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER(uint clientId, void* data, uint dataSize)
	{
		returncode = DEFAULT_RETURNCODE;
		const uint index = *((uint*)data + 2);
		missionBoardOffersMaxIndexByClient[clientId] = std::max<uint>(missionBoardOffersMaxIndexByClient[clientId], index);
		return true;
	}

	bool __stdcall Send_FLPACKET_SERVER_GFCOMPLETEMISSIONCOMPUTERLIST(uint clientId, uint base)
	{
		returncode = DEFAULT_RETURNCODE;

		// Before the Complete Packet it sent, add the custom missions to the list.
		const auto& entry = offerIdsByBase.find(base);
		if (entry != offerIdsByBase.end())
		{
			int clientRep;
			pub::Player::GetRep(clientId, clientRep);

			for (const auto offerId : entry->second)
			{
				const auto& offer = offersByOfferId.at(offerId);
				float feelings;
				pub::Reputation::GetGroupFeelingsTowards(clientRep, offer.group, feelings);
				if (feelings > minRequiredReputationForMissions)
				{
					const uint index = ++missionBoardOffersMaxIndexByClient[clientId];
					offerIndicesByClient[clientId].push_back({ index, offerId });
					SendMissionOfferToClient(clientId, offerId, offer, base, index);
				}
			}
		}
		return true;
	}

	static void ClearJobBoardClientData(const uint clientId)
	{
		offerIndicesByClient.erase(clientId);
		missionBoardOffersMaxIndexByClient.erase(clientId);
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2)
	{
		returncode = DEFAULT_RETURNCODE;
		ClearJobBoardClientData(clientId);
	}

	void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		ClearJobBoardClientData(clientId);
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		ClearJobBoardClientData(clientId);
	}
}