#pragma once
#include <FLHook.h>

namespace EngineThrottleSyncFix
{
	void __stdcall ActivateEquip(unsigned int clientId, const XActivateEquip& activateEquip);
	void __stdcall ActivateCruise(unsigned int clientId, const XActivateCruise& activateCruise);
	void __stdcall SPObjUpdate(const SSPObjUpdateInfo& updateInfo, unsigned int clientId);
	bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& packet);
}