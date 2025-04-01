#include "Main.h"
#include "MissionBoard.h"

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

	static void SendMissionOfferToClient(const uint clientId, const MissionOffer& mission, const uint base, const uint index)
	{
		char* buffer = (char*)std::malloc(1024);
		MissionPacketFirstPart* data = (MissionPacketFirstPart*)buffer;
		data->missionId = mission.id;
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
		FmtStr message(327682, 0);
		message.append_int(mission.reward);
		message.append_string(mission.text);
		pos += message.flatten(buffer + pos, 0);
		std::memcpy(buffer + pos, &mission.reward, sizeof(uint));
		pos += sizeof(uint);
		GetClientInterface()->Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER(clientId, buffer, pos);
	}

	struct MissionAcceptance
	{
		uint index;
		uint base;
		char unk1 = 1;
		ushort unk2 = 0;
		uint unk3 = 0;
	};

	static void SendMissionAcceptance(const uint clientId, const uint boardIndex, const uint base)
	{
		MissionAcceptance data;
		data.index = boardIndex;
		data.base = base;
		GetClientInterface()->Send_FLPACKET_SERVER_GFMISSIONVENDORACCEPTANCE(clientId, &data, sizeof(MissionAcceptance));
	}

	std::unordered_map<uint, uint> missionOffersMaxIndexByClient;

	std::unordered_map<uint, MissionOffer> customMissions;
	std::unordered_map<uint, std::unordered_set<uint>> customMissionIdsByBase;
	std::unordered_map<uint, std::vector<std::pair<uint, uint>>> customMissionIndicesByClient;

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

	static void RemoveMission(const uint missionId)
	{
		for (auto& baseEntry : customMissionIdsByBase)
			baseEntry.second.erase(missionId);
		customMissions.erase(missionId);
	}

	void AddCustomMission(const MissionOffer& mission, const std::vector<uint>& bases)
	{
		customMissions.insert({ mission.id, mission });
		for (const uint base : bases)
			customMissionIdsByBase[base].insert(mission.id);
	}

	void __stdcall MissionResponse(uint boardIndex, uint p2, bool p3, uint clientId)
	{
		returncode = DEFAULT_RETURNCODE;
		const auto& clientEntry = customMissionIndicesByClient.find(clientId);
		if (clientEntry != customMissionIndicesByClient.end())
		{
			for (const auto& indexEntry : clientEntry->second)
			{
				if (indexEntry.first == boardIndex)
				{
					uint base;
					pub::Player::GetBase(clientId, base);
					if (base)
						SendMissionAcceptance(clientId, indexEntry.first, base);
					// Nobody has yet taken upon the mission
					if (customMissions.contains(indexEntry.second))
					{
						SendDestroyMissionToAll(indexEntry.second);
						RemoveMission(indexEntry.second);
					}
					// Someone already removed it from the pool
					else
					{
						// Send MISSION REJECTED message
						FmtStr caption(0, 0);
						caption.begin_mad_lib(1839);
						caption.end_mad_lib();
						pub::Player::DisplayMissionMessage(clientId, caption, MissionMessageType::MissionMessageType_Type1, true);
					}
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
					return;
				}
			}
		}
	}

	bool __stdcall Send_FLPACKET_SERVER_GFMISSIONVENDORWHYEMPTY(uint clientId, uint reason)
	{
		returncode = DEFAULT_RETURNCODE;
		customMissionIndicesByClient.erase(clientId);
		missionOffersMaxIndexByClient.erase(clientId);
		return true;
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
			for (const auto customMissionId : entry->second)
			{
				const uint index = ++missionOffersMaxIndexByClient[clientId];
				customMissionIndicesByClient[clientId].push_back({ index, customMissionId });
				SendMissionOfferToClient(clientId, customMissions.at(customMissionId), base, index);
			}
		}
		return true;
	}
}