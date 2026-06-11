#pragma once

#include <FLHook.h>
#include "Plugin.h"

#include <list>
#include <string>
#include <Windows.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <numeric>
#include <unordered_map>

namespace Globals {

    //FilePaths
    const std::string PLUGIN_CONFIG_FILE = "\\flhook_plugins\\flsr.cfg";
    const std::string SENDCASHLOG_FILE = "-givecashlog.txt";
    const std::string DATADIR = "..\\DATA";
}

namespace Timers
{
    typedef void (*_TimerFunc)();

    struct TIMER {
        _TimerFunc proc;
        mstime tmIntervallMS;
        mstime tmLastCall;
    };

    int __stdcall Update();
}

namespace Commands {
    bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd);
    void UserCMD_SendCash$(uint iClientID, const std::wstring &wscParam);
    void UserCMD_SendCash(uint iClientID, const std::wstring &wscParam);
    void UserCmd_UV(uint iClientID, const std::wstring &wscParam);
    void UserCmd_HELP(uint iClientID, const std::wstring& wscParam);
    
    typedef void (*_UserCmdProc)(uint, const std::wstring &);
    struct USERCMD {
        const  wchar_t* wszCmd;
        _UserCmdProc proc;
    };

    #define IS_CMD(a) !wscCmd.compare(L##a)
}

namespace SendCash {
    static int set_iMinTime = 0;
    static int set_iMinTransfer = 0;

    void LogTransfer(std::wstring wscToCharname, std::wstring msg);
}

namespace PopUp {
    // Welcome Message - First Char on ID
    extern uint iWMsg_Head;
    extern uint iWMsg_Body;

    void WelcomeBox(uint iClientID);

    enum PopUpType {
		PopUpType_Help
	};

    //POPUPBox with Handle
    struct PopUpBox {
        uint iClientID;
        uint iHead;
        uint iBody;
        uint iPage;
		uint iMaxPage;
        uint iButton;
		PopUpType iType;
    };
}

namespace Tools {

    enum eDeathTypes {
        PVP,
        SUICIDE,
        PVE,
        KILLEDHIMSELF,
        ADMIN,
        HASDIED
    };

    void HkNewPlayerMessage(uint iClientID, struct CHARACTER_ID const &cId);
}

namespace Hooks {
    void __stdcall CharacterSelect(struct CHARACTER_ID const &cId, unsigned int iClientID);
    void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip);
    void SendDeathMsg(const std::wstring& wscMsg, uint iSystemID, uint iClientIDVictim, uint iClientIDKiller);
    }