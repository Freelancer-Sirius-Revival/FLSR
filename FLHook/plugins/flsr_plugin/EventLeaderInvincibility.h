#pragma once
#include <FLHook.h>

namespace EventLeaderInvincibility
{
	void __stdcall Elapse_Time_AFTER(float seconds);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state);
    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
}