#pragma once
#include <FLHook.h>

namespace ConnectionLimiter
{
    extern uint maxParallelConnectionsPerIpAddress;
    void __stdcall Login_After(struct SLoginInfo const& li, unsigned int clientId);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2);
}