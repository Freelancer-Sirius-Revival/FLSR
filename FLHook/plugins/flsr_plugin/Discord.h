#pragma once
#include <mutex>
#include <list>

namespace Discord
{
    struct ChatMessage
    {
        std::wstring wscCharname;
        std::wstring wscChatMessage;
    };

    extern std::mutex m_Mutex;
    extern std::list<ChatMessage> lChatMessages;

    bool LoadSettings();
    void StartUp();
}