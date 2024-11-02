﻿// AFK Plugin
// By Raikkonen

// Includes
#include "Main.h"

// Global variables
std::set<uint> afks;

void LoadSettings() { returncode = DEFAULT_RETURNCODE; }

// This text mimics the "New Player" messages
bool RedText(uint iClientID, std::wstring message, std::wstring message2) {

    std::wstring charname =
        (const wchar_t *)Players.GetActiveCharacterName(iClientID);
    uint iSystemID;
    pub::Player::GetSystem(iClientID, iSystemID);

    std::wstring wscXMLMsg =
        L"<TRA data=\"" + set_wscDeathMsgStyleSys + L"\" mask=\"-1\"/> <TEXT>";
    wscXMLMsg += XMLText(message);
    wscXMLMsg += charname;
    wscXMLMsg += XMLText(message2);
    wscXMLMsg += L"</TEXT>";

    char szBuf[0x1000];
    uint iRet;
    if (!HKHKSUCCESS(HkFMsgEncodeXML(wscXMLMsg, szBuf, sizeof(szBuf), iRet)))
        return false;

    // Send to all players in system
    struct PlayerData *pPD = 0;
    while (pPD = Players.traverse_active(pPD)) {
        uint iClientID = HkGetClientIdFromPD(pPD);
        uint iClientSystemID = 0;
        pub::Player::GetSystem(iClientID, iClientSystemID);

        if (iSystemID == iClientSystemID)
            HkFMsgSendChat(iClientID, szBuf, iRet);
    }
    return true;
}

// This command is called when a player types /afk
bool UserCmd_AFK(uint iClientID, const std::wstring &wscCmd,
                 const std::wstring &wscParam, const wchar_t *usage) {
    afks.insert(iClientID);
    RedText(iClientID, L"", L" is now away from keyboard.");
    PrintUserCmdText(
        iClientID,
        L"Use the /back command to stop sending automatic replies to PMs.");
    return true;
}

// This function welcomes the player back and removes their afk status
void Back(uint iClientID) {
    if (afks.count(iClientID) > 0) {
        afks.erase(iClientID);
        std::wstring message =
            (const wchar_t *)Players.GetActiveCharacterName(iClientID);
        RedText(iClientID, L"Welcome back ", L".");
        return;
    }
    PrintUserCmdText(
        iClientID,
        L"You are not marked as AFK. To do this, use the /afk command.");
    return;
}

// This command is called when a player types /back
bool UserCmd_Back(uint iClientID, const std::wstring &wscCmd,
                  const std::wstring &wscParam, const wchar_t *usage) {
    Back(iClientID);
    return true;
}

// Clean up when a client disconnects
void DisConnect_AFTER(uint iClientID) {
    returncode = DEFAULT_RETURNCODE;

    if (afks.count(iClientID) > 0)
        afks.erase(iClientID);
}

// Hook on chat being sent (This gets called twice with the iClientID and iTo
// swapped
void __stdcall HkCb_SendChat(uint iClientID, uint iTo, uint iSize, void *pRDL) {
    returncode = DEFAULT_RETURNCODE;

    if (HkIsValidClientID(iTo) && afks.count(iClientID) > 0)
        PrintUserCmdText(iTo, L"This user is away from keyboard.");
}

// Hooks on chat being submitted
void __stdcall SubmitChat(struct CHAT_ID cId, unsigned long lP1,
                          void const *rdlReader, struct CHAT_ID cIdTo,
                          int iP2) {
    returncode = DEFAULT_RETURNCODE;

    if (HkIsValidClientID(cId.iID) && afks.count(cId.iID))
        Back(cId.iID);
}

// Client command processing
USERCMD UserCmds[] = {
    {L"/afk", UserCmd_AFK, L"Usage: /afk"},
    {L"/back", UserCmd_Back, L"Usage: /back"},
};

bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd) {
    returncode = DEFAULT_RETURNCODE;

    std::wstring wscCmdLineLower = ToLower(wscCmd);

    // If the chat string does not match the USER_CMD then we do not handle the
    // command, so let other plugins or FLHook kick in. We require an exact
    // match
    for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++) {

        if (wscCmdLineLower.find(UserCmds[i].wszCmd) == 0) {
            // Extract the parameters string from the chat string. It should
            // be immediately after the command and a space.
            std::wstring wscParam = L"";
            if (wscCmd.length() > wcslen(UserCmds[i].wszCmd)) {
                if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
                    continue;
                wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
            }

            // Dispatch the command to the appropriate processing function.
            if (UserCmds[i].proc(iClientID, wscCmd, wscParam,
                                 UserCmds[i].usage)) {
                // We handled the command tell FL hook to stop processing this
                // chat string.
                returncode =
                    SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command,
                                                // return immediatly
                return true;
            }
        }
    }
    return false;
}

// Hook on /help
EXPORT void UserCmd_Help(uint iClientID, const std::wstring &wscParam) {
    PrintUserCmdText(iClientID, L"/afk ");
    PrintUserCmdText(iClientID,
                     L"Sets the player to AFK. If any other player messages "
                     L"directly, they will be told you are afk.");
    PrintUserCmdText(iClientID, L"/back ");
    PrintUserCmdText(iClientID, L"Turns off AFK for a the player.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Do things when the dll is loaded
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    srand((uint)time(0));
    // If we're being loaded from the command line while FLHook is running then
    // set_scCfgFile will not be empty so load the settings as FLHook only
    // calls load settings on FLHook startup and .rehash.
    if (fdwReason == DLL_PROCESS_ATTACH) {
        if (set_scCfgFile.length() > 0)
            LoadSettings();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
    }
    return true;
}

// Functions to hook
EXPORT PLUGIN_INFO *Get_PluginInfo() {
    PLUGIN_INFO *p_PI = new PLUGIN_INFO();
    p_PI->sName = "AFK";
    p_PI->sShortName = "afk";
    p_PI->bMayPause = true;
    p_PI->bMayUnload = true;
    p_PI->ePluginReturnCode = &returncode;
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&LoadSettings, PLUGIN_LoadSettings, 0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&DisConnect_AFTER,
                        PLUGIN_HkIServerImpl_DisConnect_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&UserCmd_Process,
                                             PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&UserCmd_Help, PLUGIN_UserCmd_Help, 0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&HkCb_SendChat, PLUGIN_HkCb_SendChat, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO(
        (FARPROC *)&SubmitChat, PLUGIN_HkIServerImpl_SubmitChat, 0));
    return p_PI;
}
