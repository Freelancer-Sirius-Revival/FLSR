#pragma once
#include <FLHook.h>

namespace Duel
{
	void Initialize();
	bool UserCmds(const uint clientId, const std::wstring& argumentsWS);
	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
	void __stdcall Elapse_Time_AFTER(float seconds);
	void __stdcall CharacterSelect_After(const CHARACTER_ID& cId, unsigned int clientId);
	void __stdcall DisConnect_After(unsigned int clientId, enum EFLConnection p2);
	void __stdcall BaseExit_After(unsigned int baseId, unsigned int clientId);
	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId);
}