#pragma once
#include <FLHook.h>

namespace DeathPenalty
{
	void Initialize();
	void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
	void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
	void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
	void __stdcall BaseExit_After(unsigned int baseId, unsigned int clientId);
	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
	HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
	HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
	bool UserCmds(const uint clientId, const std::wstring& argumentsWS);
	bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
}