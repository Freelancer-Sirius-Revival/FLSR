#pragma once
#include <FLHook.h>

namespace LootBoxes
{
    void ReadInitialData();
    bool UserCmd_Open(const uint clientId, const std::wstring& argumentsWS);
}