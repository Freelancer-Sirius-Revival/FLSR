#pragma once
#include <FLHook.h>

namespace Chat
{
    HK_ERROR HkSendUChat(std::wstring wscCharname, std::wstring wscText);
}