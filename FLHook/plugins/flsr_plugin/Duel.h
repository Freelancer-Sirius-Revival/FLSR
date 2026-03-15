#pragma once
#include <FLHook.h>

namespace Duel
{
	void Initialize();
	bool UserCmds(const uint clientId, const std::wstring& argumentsWS);
	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
	void __stdcall Elapse_Time_AFTER(float seconds);
}