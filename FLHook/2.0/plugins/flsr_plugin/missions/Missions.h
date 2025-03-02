#pragma once
#include <FLHook.h>

namespace Missions
{
    void LoadSettings();
    void Initialize();
    void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
}