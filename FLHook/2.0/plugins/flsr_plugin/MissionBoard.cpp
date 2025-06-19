#include "Main.h"
#include "MissionBoard.h"
#include "Missions/Missions.h"

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

	static void SendMissionOfferToClient(const uint clientId, const uint missionId, const MissionOffer& mission, const uint base, const uint index)
	{
		char* buffer = (char*)std::malloc(1024);
		MissionPacketFirstPart* data = (MissionPacketFirstPart*)buffer;
		data->missionId = missionId;
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
		message.append_fmt_str(FmtStr(mission.text, 0));
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

	static void SendMissionAcceptance(const uint clientId, const uint boardIndex, const uint base)
	{
		MissionAcceptance data;
		data.index = boardIndex;
		data.base = base;
		std::memset(&data.acceptanceData, 0, sizeof(data.acceptanceData));
		data.acceptanceData[0] = 1; // TRUE, accepted
		GetClientInterface()->Send_FLPACKET_SERVER_GFMISSIONVENDORACCEPTANCE(clientId, &data, sizeof(MissionAcceptance));
	}

	static void SendMissionRejection(const uint clientId, const uint boardIndex, const uint base, const ushort rejectionResourceId)
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

	std::unordered_map<uint, MissionOffer> customMissions;
	std::unordered_map<uint, std::unordered_set<uint>> customMissionIdsByBase;
	std::unordered_map<uint, std::vector<std::pair<uint, uint>>> customMissionIndicesByClient;
	std::unordered_map<uint, uint> missionOffersMaxIndexByClient;

	static void SendDestroyMissionToAll(const uint missionId)
	{
		for (auto& clientEntry : customMissionIndicesByClient)
		{
			for (auto indexEntry = clientEntry.second.begin(); indexEntry != clientEntry.second.end(); indexEntry++)
			{
				if (indexEntry->second == missionId)
				{
					const uint clientId = clientEntry.first;
					uint base;
					pub::Player::GetBase(clientId, base);
					if (base)
						GetClientInterface()->Send_FLPACKET_SERVER_GFDESTROYMISSIONCOMPUTER(clientId, base, missionId);
					// Do not clear customMissionIndicesByClient here. Other clients may still have the mission visible if they have the mission board open at the same time.
					break;
				}
			}
		}
	}

	uint nextMissionId = std::numeric_limits<uint>::max();

	uint AddCustomMission(const MissionOffer& mission, const std::vector<uint>& bases)
	{
		customMissions.insert({ nextMissionId, mission });
		for (const uint base : bases)
			customMissionIdsByBase[base].insert(nextMissionId);
		// To avoid collisions with existing missions, count the custom mission IDs from high to low.
		return nextMissionId--;
	}

	void DeleteCustomMission(const uint missionId)
	{
		customMissions.erase(missionId);
		for (auto& baseEntry : customMissionIdsByBase)
			baseEntry.second.erase(missionId);
		SendDestroyMissionToAll(missionId);
	}

	void __stdcall MissionResponse(uint boardIndex, uint origin, bool accepted, uint clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (!accepted)
			return;

		uint missionId;
		pub::Player::GetMsnID(clientId, missionId);
		if (missionId > 0 || Missions::IsPartOfOfferedJob(clientId))
		{
			uint base;
			pub::Player::GetBase(clientId, base);
			SendMissionRejection(clientId, boardIndex, base, 1840);
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return;
		}

		const auto& clientEntry = customMissionIndicesByClient.find(clientId);
		if (clientEntry != customMissionIndicesByClient.end())
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
					// Nobody has yet taken upon the mission
					if (customMissions.contains(indexEntry.second))
					{
						SendMissionAcceptance(clientId, boardIndex, base);
						// SetMsnID makes sure: 1. the player cannot be invited to groups ("Is in a Mission") and 2. it sets the same MsnId to group-joining players.
						pub::Player::SetMsnID(clientId, indexEntry.second, clientId, false, 0);
						Missions::StartMissionByOfferId(indexEntry.second, clientId);
						DeleteCustomMission(indexEntry.second);
					}
					// Someone already removed it from the pool
					else
					{
						SendMissionRejection(clientId, boardIndex, base, 976);
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

	bool __stdcall Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER(uint clientId, void* data, uint dataSize)
	{
		returncode = DEFAULT_RETURNCODE;
		const uint index = *((uint*)data + 2);
		missionOffersMaxIndexByClient[clientId] = std::max<uint>(missionOffersMaxIndexByClient[clientId], index);
		return true;
	}

	bool __stdcall Send_FLPACKET_SERVER_GFCOMPLETEMISSIONCOMPUTERLIST(uint clientId, uint base)
	{
		returncode = DEFAULT_RETURNCODE;

		// Before the Complete Packet it sent, add the custom missions to the list.
		const auto& entry = customMissionIdsByBase.find(base);
		if (entry != customMissionIdsByBase.end())
		{
			int clientRep;
			pub::Player::GetRep(clientId, clientRep);

			for (const auto customMissionId : entry->second)
			{
				const auto& mission = customMissions.at(customMissionId);
				float feelings;
				pub::Reputation::GetGroupFeelingsTowards(clientRep, mission.group, feelings);
				if (feelings > minRequiredReputationForMissions)
				{
					const uint index = ++missionOffersMaxIndexByClient[clientId];
					customMissionIndicesByClient[clientId].push_back({ index, customMissionId });
					SendMissionOfferToClient(clientId, customMissionId, mission, base, index);
				}
			}
		}
		return true;
	}

	static void ClearJobBoardClientData(const uint clientId)
	{
		customMissionIndicesByClient.erase(clientId);
		missionOffersMaxIndexByClient.erase(clientId);
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