#include "hook.h"
#define SPDLOG_USE_STD_FORMAT

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////



std::shared_ptr<spdlog::logger> FLHookLog = nullptr;
std::shared_ptr<spdlog::logger> CheaterLog = nullptr;
std::shared_ptr<spdlog::logger> KickLog = nullptr;
std::shared_ptr<spdlog::logger> ConnectsLog = nullptr;
std::shared_ptr<spdlog::logger> AdminCmdsLog = nullptr;
std::shared_ptr<spdlog::logger> SocketCmdsLog = nullptr;
std::shared_ptr<spdlog::logger> UserCmdsLog = nullptr;
std::shared_ptr<spdlog::logger> UserChatLog = nullptr;
std::shared_ptr<spdlog::logger> PerfTimersLog = nullptr;
std::shared_ptr<spdlog::logger> FLHookDebugLog = nullptr;
std::shared_ptr<spdlog::logger> WinDebugLog = nullptr;


bool InitLogs()
{
    try
    {
        FLHookLog = spdlog::basic_logger_mt<spdlog::async_factory>("FLHook", "logs/FLHook.log");
        CheaterLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_cheaters", "logs/flhook_cheaters.log");
        KickLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_kicks", "logs/flhook_kicks.log");
        ConnectsLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_connects", "logs/flhook_connects.log");
        AdminCmdsLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_admincmds", "logs/flhook_admincmds.log");
        SocketCmdsLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_socketcmds", "logs/flhook_socketcmds.log");
        UserCmdsLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_usercmds", "logs/flhook_usercmds.log");
        UserChatLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_userchat", "logs/flhook_userchat.log");
        PerfTimersLog = spdlog::basic_logger_mt<spdlog::async_factory>("flhook_perftimers", "logs/flhook_perftimers.log");

        spdlog::flush_on(spdlog::level::err);
        spdlog::flush_every(std::chrono::seconds(3));

        if (IsDebuggerPresent())
        {
            WinDebugLog = spdlog::create_async<spdlog::sinks::msvc_sink_mt>("windows_debug");
            WinDebugLog->set_level(spdlog::level::debug);
        }

        if (fLogDebug)
        {
            char szDate[64];
            time_t tNow = time(nullptr);
            tm t;
            localtime_s(&t, &tNow);
            strftime(szDate, sizeof szDate, "%d.%m.%Y_%H.%M", &t);

            std::string sDebugLog = "./logs/debug/FLHookDebug_" + (std::string)szDate;
            sDebugLog += ".log";

            FLHookDebugLog = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", sDebugLog);
            FLHookDebugLog->set_level(spdlog::level::debug);
        }
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        ConPrint(L"ERROR FAILED TO RUN LOGGER");
        return false;
    }
    return true;
}

void AddLog_s(LogType LogType, LogLevel lvl, const std::string& str)
{
    auto level = static_cast<spdlog::level::level_enum>(lvl);

    switch (LogType)
    {
    case LogType::Chat:
        UserChatLog->log(level, str);
        break;
    case LogType::Cheater:
        CheaterLog->log(level, str);
        break;
    case LogType::Kick:
        KickLog->log(level, str);
        break;
    case LogType::Connects:
        ConnectsLog->log(level, str);
        break;
    case LogType::AdminCmds:
        AdminCmdsLog->log(level, str);
        break;
    case LogType::UserLogCmds:
        UserCmdsLog->log(level, str);
        break;
    case LogType::SocketCmds:
        SocketCmdsLog->log(level, str);
        break;
    case LogType::PerfTimers:
        PerfTimersLog->log(level, str);
        break;
    case LogType::Normal:
        switch (level)
        {
        case spdlog::level::debug:
        //    ConPrint(L"debug logs!! \n");
            break;
        case spdlog::level::info:
            //Console::ConInfo(str);
            break;
        case spdlog::level::warn:
           // Console::ConWarn(str);
            break;
        case spdlog::level::critical:
        case spdlog::level::err:
           // Console::ConErr(str);
            break;
        default:;
        }

        FLHookLog->log(level, str);
        break;
    default:
        break;
    }

    if (lvl == LogLevel::Debug && FLHookDebugLog)
    {
        FLHookDebugLog->debug(str);
    }

    if (IsDebuggerPresent() && WinDebugLog)
    {
        WinDebugLog->debug(str);
    }

    if (lvl == LogLevel::Critical)
    {
        // Ensure all is flushed!
        spdlog::shutdown();
    }
}

void AddDebugLog(const std::string szString, ...) {
 
    char buffer[1024];
    va_list args;
    va_start(args, szString);
    vsprintf_s(buffer, 1024, szString.c_str(), args);
    va_end(args);
    AddLog_s(LogType::Normal, LogLevel::Debug, std::string(buffer));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddLog(const std::string szString, ...) {

    char buffer[1024];
    va_list args;
    va_start(args, szString);
    vsprintf_s(buffer, 1024, szString.c_str(), args);
    va_end(args);
    AddLog_s(LogType::Normal, LogLevel::Info, std::string(buffer));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HkHandleCheater(uint iClientID, bool bBan, std::wstring wscReason, ...) {
   // wchar_t wszBuf[1024 * 8] = L"";
    //va_list marker;
    //va_start(marker, wscReason);

   // _vsnwprintf_s(wszBuf, (sizeof(wszBuf) / 2) - 1, wscReason.c_str(), marker);

   HkAddCheaterLog(iClientID, wscReason);
    
    if (wscReason[0] != '#' && Players.GetActiveCharacterName(iClientID)) {
        std::wstring wscCharname =
            (wchar_t *)Players.GetActiveCharacterName(iClientID);

       // wchar_t wszBuf2[500];
        /*swprintf_s(wszBuf2, L"Possible cheating detected: %s",
                   wscCharname.c_str());*/
        
        HkMsgU(std::format(L"Possible cheating detected: {}", wscCharname.c_str()));
    }

    if (bBan)
        HkBan(ARG_CLIENTID(iClientID), true);
    if (wscReason[0] != '#')
        HkKick(ARG_CLIENTID(iClientID));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HkAddCheaterLog(const std::wstring &wscCharname,
                     const std::wstring &wscReason) {
  /*  FILE* f;
    fopen_s(&f, ("./flhook_logs/flhook_cheaters.log"), "at");
    if (!f)
        return false;*/

    CAccount *acc = HkGetAccountByCharname(wscCharname);
    std::wstring wscAccountDir = L"???";
    std::wstring wscAccountID = L"???";
    if (acc) {
        HkGetAccountDirName(acc, wscAccountDir);
        wscAccountID = HkGetAccountID(acc);
    }

    uint iClientID = HkGetClientIdFromCharname(wscCharname);
    std::wstring wscHostName = L"???";
    std::wstring wscIp = L"???";
    if (iClientID != -1) {
        wscHostName = ClientInfo[iClientID].wscHostname;
        HkGetPlayerIP(iClientID, wscIp);
    }

    /*time_t tNow = time(nullptr);
    tm stNow;
    localtime_s(&stNow, &tNow);
    fprintf(f,
            "%.2d/%.2d/%.4d %.2d:%.2d:%.2d Possible cheating detected (%s) by "
            "%s(%s)(%s) [%s %s]\n",
            stNow.tm_mon + 1, stNow.tm_mday, stNow.tm_year + 1900,
            stNow.tm_hour, stNow.tm_min, stNow.tm_sec, wstos(wscReason).c_str(),
            wstos(wscCharname).c_str(), wstos(wscAccountDir).c_str(),
            wstos(wscAccountID).c_str(), wstos(wscHostName).c_str(),
            wstos(wscIp).c_str());
    fclose(f);*/
    AddLog_s(LogType::Cheater, LogLevel::Info, wstos(std::format(L"Possible cheating detected ({}) by {}({})({}) [{} {}]",
        wscReason, wscCharname, wscAccountDir.c_str(), wscAccountID.c_str(), wscHostName.c_str(), wscIp.c_str())));


    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HkAddCheaterLog(const uint &iClientID, const std::wstring &wscReason) {
   /* FILE* f;
    fopen_s(&f, ("./flhook_logs/flhook_cheaters.log"), "at");
    if (!f)
        return false;*/

    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    std::wstring wscAccountDir = L"???";
    std::wstring wscAccountID = L"???";
    if (acc) {
        HkGetAccountDirName(acc, wscAccountDir);
        wscAccountID = HkGetAccountID(acc);
    }

    std::wstring wscHostName = L"???";
    std::wstring wscIp = L"???";

    wscHostName = ClientInfo[iClientID].wscHostname;
    HkGetPlayerIP(iClientID, wscIp);

    std::wstring wscCharname =
        L"? ? ?"; // spaces to make clear it's not a player name
    if (Players.GetActiveCharacterName(iClientID)) {
        wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
    }

   /* time_t tNow = time(0);
    tm stNow;
    localtime_s(&stNow, &tNow);
    fprintf(f,
            "%.2d/%.2d/%.4d %.2d:%.2d:%.2d Possible cheating detected (%s) by "
            "%s(%s)(%s) [%s %s]\n",
            stNow.tm_mon + 1, stNow.tm_mday, stNow.tm_year + 1900,
            stNow.tm_hour, stNow.tm_min, stNow.tm_sec, wstos(wscReason).c_str(),
            wstos(wscCharname).c_str(), wstos(wscAccountDir).c_str(),
            wstos(wscAccountID).c_str(), wstos(wscHostName).c_str(),
            wstos(wscIp).c_str());
    fclose(f);*/

    AddLog_s(LogType::Cheater, LogLevel::Info, wstos(std::format(L"Possible cheating detected ({}) by {}({})({}) [{} {}]",
        wscReason, wscCharname.c_str(), wscAccountDir.c_str(), wscAccountID.c_str(), wscHostName.c_str(), wscIp.c_str())));

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HkAddKickLog(uint iClientID, std::wstring wscReason, ...) {
   /* wchar_t wszBuf[1024 * 8] = L"";
    va_list marker;
    va_start(marker, wscReason);

    _vsnwprintf_s(wszBuf, (sizeof(wszBuf) / 2) - 1, wscReason.c_str(), marker);

    FILE *f;
    fopen_s(&f, ("./flhook_logs/flhook_kicks.log"), "at");
    if (!f)
        return false;
        */

    const wchar_t *wszCharname =
        (wchar_t *)Players.GetActiveCharacterName(iClientID);
    if (!wszCharname)
        wszCharname = L"";

    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    std::wstring wscAccountDir;
    HkGetAccountDirName(acc, wscAccountDir);

   /* time_t tNow = time(0);
    tm stNow;
    localtime_s(&stNow, &tNow);
    fprintf(f, "%.2d/%.2d/%.4d %.2d:%.2d:%.2d Kick (%s): %s(%s)(%s)\n",
            stNow.tm_mon + 1, stNow.tm_mday, stNow.tm_year + 1900,
            stNow.tm_hour, stNow.tm_min, stNow.tm_sec, wstos(wszBuf).c_str(),
            wstos(wszCharname).c_str(), wstos(wscAccountDir).c_str(),
            wstos(HkGetAccountID(acc)).c_str());
    fclose(f);*/

    AddLog_s(LogType::Kick, LogLevel::Info, wstos(std::format(L"Kick ({}): {}({})({})\n", wscReason.c_str(), wszCharname, wscAccountDir.c_str(), HkGetAccountID(acc).c_str())));
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HkAddConnectLog(uint iClientID, std::wstring wscReason, ...) {
   /* wchar_t wszBuf[1024 * 8] = L"";
    va_list marker;
    va_start(marker, wscReason);

    _vsnwprintf_s(wszBuf, (sizeof(wszBuf) / 2) - 1, wscReason.c_str(), marker);

    FILE *f;
    fopen_s(&f, ("./flhook_logs/flhook_connects.log"), "at");
    if (!f)
        return false;*/

    const wchar_t *wszCharname =
        (wchar_t *)Players.GetActiveCharacterName(iClientID);
    if (!wszCharname)
        wszCharname = L"";

    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    std::wstring wscAccountDir;
    HkGetAccountDirName(acc, wscAccountDir);

   /* time_t tNow = time(0);
    tm stNow;
    localtime_s(&stNow, &tNow);
    fprintf(f, "%.2d/%.2d/%.4d %.2d:%.2d:%.2d Connect (%s): %s(%s)(%s)\n",
            stNow.tm_mon + 1, stNow.tm_mday, stNow.tm_year + 1900,
            stNow.tm_hour, stNow.tm_min, stNow.tm_sec, wstos(wszBuf).c_str(),
            wstos(wszCharname).c_str(), wstos(wscAccountDir).c_str(),
            wstos(HkGetAccountID(acc)).c_str());
    fclose(f);*/

    AddLog_s(LogType::Connects, LogLevel::Info, wstos(std::format(L"Connect ({}): {}({})({})\n", wscReason.c_str(), wszCharname, wscAccountDir.c_str(), HkGetAccountID(acc).c_str())));
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void AddLog(FILE *fLog, const char *szString, ...) {
    char szBufString[1024];
    va_list marker;
    va_start(marker, szString);
    _vsnprintf_s(szBufString, sizeof(szBufString) - 1, szString, marker);

    char szBuf[64];
    time_t tNow = time(0);
    tm t;
    localtime_s(&t, &tNow);
    strftime(szBuf, sizeof(szBuf), "%d.%m.%Y %H:%M:%S", &t);
    fprintf(fLog, "[%s] %s\n", szBuf, szBufString);
    fflush(fLog);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HkAddAdminCmdLog(std::string scString, ...) {

    AddLog_s(LogType::AdminCmds, LogLevel::Info, "AdminCmd (" + scString + ")\n");


    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HkAddSocketCmdLog(std::string scString, ...) {

    AddLog_s(LogType::UserLogCmds, LogLevel::Info, "SocketCmd (" + scString + ")\n");


    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HkAddUserCmdLog(std::string scString, ...) {
   

    AddLog_s(LogType::UserLogCmds, LogLevel::Info, "UserCmd (" + scString + ")\n");


    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HkAddPerfTimerLog(std::string scString, ...) {

    AddLog_s(LogType::PerfTimers, LogLevel::Info, "PerfTimer (" + scString + ")\n");


    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HkAddUserChatLog(std::string scString, ...) {

    AddLog_s(LogType::Chat, LogLevel::Info, "Chat (" + scString + ")\n");


    return;
}
