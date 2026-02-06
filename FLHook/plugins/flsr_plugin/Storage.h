#pragma once
#include <FLHook.h>

namespace Storage
{
    void Initialize();
	void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
	void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
	HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
	HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    bool UserCmd_Storage(const uint clientId, const std::wstring& arguments);
}