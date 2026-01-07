#pragma once
#include <FLHook.h>

namespace NameLimiter
{
	void __stdcall CreateNewCharacter(const SCreateCharacterInfo& scci, unsigned int clientId);
}