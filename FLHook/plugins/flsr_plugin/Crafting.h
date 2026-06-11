#pragma once
#include <FLHook.h>

namespace Crafting
{
    void LoadSettings();
    bool UserCmd_Craft(const uint clientId, const std::wstring& argumentsWS);
}
