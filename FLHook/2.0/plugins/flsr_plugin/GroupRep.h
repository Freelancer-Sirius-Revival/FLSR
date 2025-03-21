#pragma once
#include <FLHook.h>

namespace GroupReputation
{
	void __stdcall ObjectDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
}