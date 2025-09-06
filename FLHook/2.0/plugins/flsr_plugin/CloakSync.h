#pragma once
#include <FLHook.h>

namespace CloakSync
{
	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet);
	bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& packet);
}