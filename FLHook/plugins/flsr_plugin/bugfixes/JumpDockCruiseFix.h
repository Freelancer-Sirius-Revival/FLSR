#pragma once
#include <FLHook.h>

namespace JumpDockCruiseFix
{
	int __cdecl Dock_Call_After(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, DOCK_HOST_RESPONSE response);
	void __stdcall JumpInComplete_After(unsigned int systemId, unsigned int shipId);
}