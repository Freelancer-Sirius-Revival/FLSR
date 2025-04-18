#pragma once
#include <FLHook.h>

namespace MissionBoard
{
	struct MissionOffer
	{
		pub::GF::MissionType type;
		uint system;
		uint group;
		uint text;
		uint reward;
	};

	uint AddCustomMission(const MissionOffer& mission, const std::vector<uint>& bases);
	void DeleteCustomMission(const uint missionId);

	void Initialize();
	void __stdcall MissionResponse(uint missionId, uint origin, bool accepted, uint clientId);
	void __stdcall AbortMission(uint clientId, uint missionId);
	bool __stdcall Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER(uint clientId, void* data, uint dataSize);
	bool __stdcall Send_FLPACKET_SERVER_GFCOMPLETEMISSIONCOMPUTERLIST(uint clientId, uint base);
	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2);
	void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId);
	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
}