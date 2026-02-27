#pragma once
#include <FLHook.h>

namespace PlayerLootSpawning
{
	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
}