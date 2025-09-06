#pragma once
#include <FLHook.h>

namespace CloakSync
{
	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet);
	bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet);
	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId);
}