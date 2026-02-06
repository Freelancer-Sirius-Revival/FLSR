#pragma once
#include <FLHook.h>

namespace Insurance
{
	void Initialize();
	void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
	void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
	void __stdcall CharacterSelect_After(const CHARACTER_ID& cId, unsigned int clientId);
	void __stdcall DisConnect_After(unsigned int clientId, enum EFLConnection p2);
	void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
	void __stdcall BaseExit_After(unsigned int baseId, unsigned int clientId);
	void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId);
	HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
	HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
	bool UserCmds(const uint clientId, const std::wstring& argumentsWS);
}