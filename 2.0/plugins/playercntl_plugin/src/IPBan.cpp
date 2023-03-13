﻿// Player Control plugin for FLHookPlugin
// Feb 2010 by Cannon
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

#include <FLHook.h>
#include <float.h>
#include <list>
#include <math.h>
#include <plugin.h>
#include <set>
#include <stdio.h>
#include <string>
#include <time.h>
#include <windows.h>

#include "Main.h"

#include "wildcards.h"

namespace IPBans {
/// list of bans
static std::list<std::string> set_lstIPBans;
static std::list<std::string> set_lstLoginIDBans;

struct INFO {
    bool bIPChecked;
};
static std::map<uint, INFO> mapInfo;

/// Return true if this client is on a banned IP range.
static bool IsBanned(uint iClientID) {
    std::wstring wscIP;
    HkGetPlayerIP(iClientID, wscIP);
    std::string scIP = wstos(wscIP);

    // Check for an IP range match.
    for (auto &ban : set_lstIPBans)
        if (Wildcard::wildcardfit(ban.c_str(), scIP.c_str()))
            return true;
    // To avoid plugin comms with DSAce because I ran out of time to make this
    // work, I use something of a trick to get the login ID.
    // Read all login ID files in the account and look for the one with a
    // matching IP to this player. If we find a matching IP then we've got a
    // login ID we can check.
    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    if (acc) {
        bool bBannedLoginID = false;

        std::wstring wscDir;
        HkGetAccountDirName(acc, wscDir);

        WIN32_FIND_DATA findFileData;

        std::string scFileSearchPath =
            scAcctPath + "\\" + wstos(wscDir) + "\\login_*.ini";
        HANDLE hFileFind =
            FindFirstFile(scFileSearchPath.c_str(), &findFileData);
        if (hFileFind != INVALID_HANDLE_VALUE) {
            do {
                // Read the login ID and IP from the login ID record.
                std::string scLoginID = "";
                std::string scLoginID2 = "";
                std::string scThisIP = "";
                std::string scFilePath =
                    scAcctPath + wstos(wscDir) + "\\" + findFileData.cFileName;
                FILE *f;
                fopen_s(&f, scFilePath.c_str(), "r");
                if (f) {
                    char szBuf[200];
                    if (fgets(szBuf, sizeof(szBuf), f) != NULL) {
                        std::string sz = szBuf;
                        try {
                            scLoginID = Trim(GetParam(sz, '\t', 1)
                                                 .substr(3, std::string::npos));
                            scThisIP = Trim(GetParam(sz, '\t', 2)
                                                .substr(3, std::string::npos));
                            if (GetParam(sz, '\t', 3).length() > 4)
                                scLoginID2 =
                                    Trim(GetParam(sz, '\t', 3)
                                             .substr(4, std::string::npos));
                        } catch (...) {
                            ConPrint(L"ERR Corrupt loginid file $0\n",
                                     stows(scFilePath).c_str());
                        }
                    }
                    fclose(f);
                }

                if (set_iPluginDebug > 2) {
                    ConPrint(L"NOTICE: Checking for ban on IP %s Login ID1 %s "
                             L"ID2 %s "
                             L"Client %d\n",
                             stows(scThisIP).c_str(), stows(scLoginID).c_str(),
                             stows(scLoginID2).c_str(), iClientID);
                }

                // If the login ID has been read then check it to see if it has
                // been banned
                if (scThisIP == scIP && scLoginID.length()) {
                    for (auto &ban : set_lstLoginIDBans) {
                        if (ban == scLoginID || ban == scLoginID2) {
                            ConPrint(L"* Kicking player on ID ban: ip=%s "
                                     L"id1=%s id2=%s\n",
                                     stows(scThisIP).c_str(),
                                     stows(scLoginID).c_str(),
                                     stows(scLoginID2).c_str());
                            bBannedLoginID = true;
                            break;
                        }
                    }
                }
            } while (FindNextFile(hFileFind, &findFileData));
            FindClose(hFileFind);
        }

        if (bBannedLoginID)
            return true;
    }
    return false;
}

/// Return true if this client is has a "Authenticated" file in the
/// account directory indicating that the client can connect even if
/// they are otherwise on a restricted IP range.
static bool IsAuthenticated(uint iClientID) {
    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    if (!acc)
        return false;

    std::wstring wscDir;
    HkGetAccountDirName(acc, wscDir);
    std::string scUserFile = scAcctPath + wstos(wscDir) + "\\authenticated";
    FILE *fTest;
    fopen_s(&fTest, scUserFile.c_str(), "r");
    if (!fTest)
        return false;

    fclose(fTest);
    return true;
}

static void ReloadIPBans() {
    // init variables
    char szDataPath[MAX_PATH];
    GetUserDataPath(szDataPath);
    std::string scAcctPath =
        std::string(szDataPath) + "\\Accts\\MultiPlayer\\ipbans.ini";
    if (set_iPluginDebug)
        ConPrint(L"NOTICE: Loading IP bans from %s\n",
                 stows(scAcctPath).c_str());

    INI_Reader ini;
    set_lstIPBans.clear();
    if (ini.open(scAcctPath.c_str(), false)) {
        while (ini.read_header()) {
            while (ini.read_value()) {
                set_lstIPBans.push_back(ini.get_name_ptr());
                if (set_iPluginDebug)
                    ConPrint(L"NOTICE: Adding IP ban %s\n",
                             stows(ini.get_name_ptr()).c_str());
            }
        }
        ini.close();
    }
    ConPrint(L"IP Bans [%u]\n", set_lstIPBans.size());
}

static void ReloadLoginIDBans() {
    // init variables
    char szDataPath[MAX_PATH];
    GetUserDataPath(szDataPath);
    std::string scAcctPath =
        std::string(szDataPath) + "\\Accts\\MultiPlayer\\loginidbans.ini";
    if (set_iPluginDebug)
        ConPrint(L"NOTICE: Loading Login ID bans from %s\n",
                 stows(scAcctPath).c_str());

    INI_Reader ini;
    set_lstLoginIDBans.clear();
    if (ini.open(scAcctPath.c_str(), false)) {
        while (ini.read_header()) {
            while (ini.read_value()) {
                set_lstLoginIDBans.push_back(
                    Trim(std::string(ini.get_name_ptr())));
                if (set_iPluginDebug)
                    ConPrint(L"NOTICE: Adding Login ID ban %s\n",
                             stows(ini.get_name_ptr()).c_str());
            }
        }
        ini.close();
    }
    ConPrint(L"ID Bans [%u]\n", set_lstLoginIDBans.size());
}

/// Reload the ipbans file.
void IPBans::LoadSettings(const std::string &scPluginCfgFile) {
    ReloadIPBans();
    ReloadLoginIDBans();
}

void IPBans::PlayerLaunch(unsigned int iShip, unsigned int iClientID) {
    if (!mapInfo[iClientID].bIPChecked) {
        mapInfo[iClientID].bIPChecked = true;
        if (IsBanned(iClientID) && !IsAuthenticated(iClientID)) {
            HkAddKickLog(iClientID, L"IP banned");
            HkMsgAndKick(iClientID,
                         L"Your IP is banned, please contact an administrator",
                         15000L);
        }
    }
}

void IPBans::BaseEnter(unsigned int iBaseID, unsigned int iClientID) {
    if (!mapInfo[iClientID].bIPChecked) {
        mapInfo[iClientID].bIPChecked = true;
        if (IsBanned(iClientID) && !IsAuthenticated(iClientID)) {
            HkAddKickLog(iClientID, L"IP banned");
            HkMsgAndKick(iClientID,
                         L"Your IP is banned, please contact an administrator",
                         7000L);
        }
    }
}

/** Start automatic zone checking */
void IPBans::AdminCmd_AuthenticateChar(CCmds *cmds,
                                       const std::wstring &wscCharname) {
    if (!(cmds->rights & RIGHT_SUPERADMIN)) {
        cmds->Print(L"ERR No permission\n");
        return;
    }

    // init variables
    char szDataPath[MAX_PATH];
    GetUserDataPath(szDataPath);

    std::wstring wscDir;
    if (HkGetAccountDirName(wscCharname, wscDir) != HKE_OK) {
        cmds->Print(L"ERR Account not found\n");
        return;
    }

    std::string scPath = std::string(szDataPath) + "\\Accts\\MultiPlayer\\" +
                         wstos(wscDir) + "\\authenticated";
    FILE *fTest;
    fopen_s(&fTest, scPath.c_str(), "w");
    if (!fTest) {
        cmds->Print(L"ERR Writing authentication file\n");
        return;
    }

    fclose(fTest);
    cmds->Print(L"OK\n");
}

void IPBans::ClearClientInfo(uint iClientID) {
    mapInfo[iClientID].bIPChecked = false;
}

void IPBans::AdminCmd_ReloadBans(CCmds *cmds) {
    ReloadLoginIDBans();
    ReloadIPBans();
    cmds->Print(L"OK\n");
}
} // namespace IPBans
