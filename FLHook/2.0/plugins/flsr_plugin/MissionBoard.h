#pragma once
#include <FLHook.h>

namespace MissionBoard
{
	struct MissionOffer
	{
		uint id;
		pub::GF::MissionType type;
		uint system;
		uint group;
		uint text;
		uint reward;
	};

	void AddCustomMission(const MissionOffer& mission, const std::vector<uint>& bases);

	void __stdcall MissionResponse(uint missionId, uint p2, bool p3, uint clientId);
	bool __stdcall Send_FLPACKET_SERVER_GFMISSIONVENDORWHYEMPTY(uint clientId, uint reason);
	bool __stdcall Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER(uint clientId, void* data, uint dataSize);
	bool __stdcall Send_FLPACKET_SERVER_GFCOMPLETEMISSIONCOMPUTERLIST(uint clientId, uint base);
	bool __stdcall Send_FLPACKET_SERVER_GFDESTROYMISSIONCOMPUTER(uint clientId, uint base, uint missionId);
}