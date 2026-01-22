#pragma once
#include <FLHook.h>
#include "Plugin.h"

namespace MissionAbortFix
{
	void __stdcall CharacterSelect(const CHARACTER_ID& cId, unsigned int clientId);
	void __stdcall CharacterSelect_AFTER(const CHARACTER_ID& cId, unsigned int clientId);
	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2);
}