#pragma once
#include <FLHook.h>

namespace NameLimiter
{
	bool IsCharacterNameAllowed(const std::string& characterName);
	void __stdcall CreateNewCharacter(const SCreateCharacterInfo& scci, unsigned int clientId);
}