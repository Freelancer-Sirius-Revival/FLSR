#pragma once
#include <FLHook.h>

namespace Missions
{
    void LoadSettings();
    void Initialize();
    void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
    void __stdcall Elapse_Time_AFTER(float seconds);
    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
}