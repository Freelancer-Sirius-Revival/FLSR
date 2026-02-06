#include "Main.h"
#include "NameLimiter.h"
#include "IFF.h"
#include "Carrier.h"
#include "Cloak.h"
#include "Mark.h"

namespace Commands
{
    void UserCmd_HELP(uint iClientID, const std::wstring& wscParam) {


        //Create Popup struct
        PopUp::PopUpBox NewPopUpBox;
        NewPopUpBox.iClientID = iClientID;
        NewPopUpBox.iHead = 524291;
        NewPopUpBox.iBody = 524292;
        NewPopUpBox.iPage = 1;
        NewPopUpBox.iMaxPage = 1;
        NewPopUpBox.iButton = POPUPDIALOG_BUTTONS_CENTER_OK;

        std::wstring wscCharFileName;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);

        //Setup New Popup
        PopUp::mPopUpBox[wscCharFileName] = NewPopUpBox;

        //OpenPopup
        PopUp::OpenPopUp(iClientID);

    }

    void UserCmd_UV(uint iClientID, const std::wstring& wscParam) {
        std::wstring Chat = wscParam;
        std::wstring Charname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
        Chat::HkSendUChat(Charname, Chat);

        //Discord
        Discord::ChatMessage ChatMsg;
        ChatMsg.wscCharname = Charname;
        ChatMsg.wscChatMessage = Chat;


        {
            // Mutex sperren
            std::lock_guard<std::mutex> lock(m_Mutex);

            // Chat-Nachricht zur Liste hinzuf�gen
            Discord::lChatMessages.push_back(ChatMsg);
        } // Mutex wird hier automatisch freigegeben


    }

    /** Process a give cash command */
    void UserCMD_SendCash(uint iClientID, const std::wstring& wscParam) {
        // The last error.
        HK_ERROR err;

        // Get the current character name
        std::wstring wscCharname =
            (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        // Get the parameters from the user command.
        std::wstring wscTargetCharname = GetParam(wscParam, L' ', 0);
        std::wstring wscCash = GetParam(wscParam, L' ', 1);
        std::wstring wscAnon = GetParam(wscParam, L' ', 2);
        wscCash = ReplaceStr(wscCash, L".", L"");
        wscCash = ReplaceStr(wscCash, L",", L"");
        wscCash = ReplaceStr(wscCash, L"$", L"");
        int cash = ToInt(wscCash);
        if ((!wscTargetCharname.length() || cash <= 0) ||
            (wscAnon.size() && wscAnon != L"anon")) {
            PrintUserCmdText(iClientID, L"ERR Invalid parameters");
            PrintUserCmdText(iClientID, L"/sendcash <charname> <cash> [anon]");
            return;
        }

        bool bAnon = false;
        if (wscAnon == L"anon")
            bAnon = true;

        if (HkGetAccountByCharname(wscTargetCharname) == 0) {
            PrintUserCmdText(iClientID, L"ERR char does not exist");
            return;
        }

        int secs = 0;
        HkGetOnlineTime(wscCharname, secs);
        if (secs < SendCash::set_iMinTime) {
            PrintUserCmdText(iClientID, L"ERR insufficient time online");
            return;
        }

        // Read the current number of credits for the player
        // and check that the character has enough cash.
        int iCash = 0;
        if ((err = HkGetCash(wscCharname, iCash)) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR " + HkErrGetText(err));
            return;
        }
        if (cash < SendCash::set_iMinTransfer || cash < 0) {
            PrintUserCmdText(iClientID,
                L"ERR Transfer too small, minimum transfer " +
                ToMoneyStr(SendCash::set_iMinTransfer) +
                L" credits");
            return;
        }
        if (iCash < cash) {
            PrintUserCmdText(iClientID, L"ERR Insufficient credits");
            return;
        }

        // Prevent target ship from becoming corrupt.
        float fTargetValue = 0.0f;
        if ((err = HKGetShipValue(wscTargetCharname, fTargetValue)) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR " + HkErrGetText(err));
            return;
        }
        if ((fTargetValue + cash) > 2000000000.0f) {
            PrintUserCmdText(iClientID, L"ERR Transfer will exceed credit limit");
            return;
        }

        // Calculate the new cash
        int iExpectedCash = 0;
        if ((err = HkGetCash(wscTargetCharname, iExpectedCash)) != HKE_OK) {
            PrintUserCmdText(iClientID,
                L"ERR Get cash failed err=" + HkErrGetText(err));
            return;
        }
        iExpectedCash += cash;

        // Do an anticheat check on the receiving character first.
        uint targetClientId = HkGetClientIdFromCharname(wscTargetCharname);
        if (targetClientId != -1 && !HkIsInCharSelectMenu(targetClientId)) {
            if (HkAntiCheat(targetClientId) != HKE_OK) {
                PrintUserCmdText(iClientID, L"ERR Transfer failed");
                AddLog(
                    "NOTICE: Possible cheating when sending %s credits from %s "
                    "(%s) "
                    "to %s (%s)",
                    wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname)))
                    .c_str(),
                    wstos(wscTargetCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                    .c_str());
                return;
            }
            HkSaveChar(targetClientId);
        }

        if (targetClientId != -1) {
            if (HkIsValidClientID(ClientInfo[iClientID].iTradePartner) ||
                HkIsValidClientID(ClientInfo[targetClientId].iTradePartner)) {
                PrintUserCmdText(iClientID, L"ERR Trade window open");
                AddLog(
                    "NOTICE: Trade window open when sending %s credits from %s "
                    "(%s) "
                    "to %s (%s) %u %u",
                    wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname)))
                    .c_str(),
                    wstos(wscTargetCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                    .c_str(),
                    iClientID, targetClientId);
                return;
            }
        }

        // Remove cash from current character and save it checking that the
        // save completes before allowing the cash to be added to the target ship.
        if ((err = HkAddCash(wscCharname, 0 - cash)) != HKE_OK) {
            PrintUserCmdText(iClientID,
                L"ERR Remove cash failed err=" + HkErrGetText(err));
            return;
        }

        if (HkAntiCheat(iClientID) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR Transfer failed");
            AddLog(
                "NOTICE: Possible cheating when sending %s credits from %s (%s) to "
                "%s (%s)",
                wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname))).c_str(),
                wstos(wscTargetCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                .c_str());
            return;
        }
        HkSaveChar(iClientID);

        // Add cash to target character
        if ((err = HkAddCash(wscTargetCharname, cash)) != HKE_OK) {
            PrintUserCmdText(iClientID,
                L"ERR Add cash failed err=" + HkErrGetText(err));
            return;
        }

        targetClientId = HkGetClientIdFromCharname(wscTargetCharname);
        if (targetClientId != -1 && !HkIsInCharSelectMenu(targetClientId)) {
            if (HkAntiCheat(targetClientId) != HKE_OK) {
                PrintUserCmdText(iClientID, L"ERR Transfer failed");
                AddLog(
                    "NOTICE: Possible cheating when sending %s credits from %s "
                    "(%s) "
                    "to %s (%s)",
                    wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname)))
                    .c_str(),
                    wstos(wscTargetCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                    .c_str());
                return;
            }
            HkSaveChar(targetClientId);
        }

        // Check that receiving character has the correct ammount of cash.
        int iCurrCash;
        if ((err = HkGetCash(wscTargetCharname, iCurrCash)) != HKE_OK ||
            iCurrCash != iExpectedCash) {
            AddLog(
                "ERROR: Cash transfer error when sending %s credits from %s (%s) "
                "to "
                "%s (%s) current %s credits expected %s credits ",
                wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname))).c_str(),
                wstos(wscTargetCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                .c_str(),
                wstos(ToMoneyStr(iCurrCash)).c_str(),
                wstos(ToMoneyStr(iExpectedCash)).c_str());
            PrintUserCmdText(iClientID, L"ERR Transfer failed");
            return;
        }

        // If the target player is online then send them a message saying
        // telling them that they've received the cash.
        std::wstring msg = L"You have received " + ToMoneyStr(cash) +
            L" credits from " +
            ((bAnon) ? L"anonymous" : wscCharname);
        if (targetClientId != -1 && !HkIsInCharSelectMenu(targetClientId)) {
            PrintUserCmdText(targetClientId, L"%s", msg.c_str());
        }
        // Otherwise we assume that the character is offline so we record an entry
        // in the character's givecash.ini. When they come online we inform them
        // of the transfer. The ini is cleared when ever the character logs in.
        else {
            std::wstring msg = L"You have received " + ToMoneyStr(cash) +
                L" credits from " +
                ((bAnon) ? L"anonymous" : wscCharname);
            SendCash::LogTransfer(wscTargetCharname, msg);
        }

        AddLog("Send %s credits from %s (%s) to %s (%s)",
            wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
            wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname))).c_str(),
            wstos(wscTargetCharname).c_str(),
            wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
            .c_str());

        // A friendly message explaining the transfer.
        msg = L"You have sent " + ToMoneyStr(cash) + L" credits to " +
            wscTargetCharname;
        if (bAnon)
            msg += L" anonymously";
        PrintUserCmdText(iClientID, L"%s", msg.c_str());
        return;
    }

    void UserCMD_SendCash$(uint iClientID, const std::wstring& wscParam) {
        // The last error.
        HK_ERROR err;

        // Get the current character name
        std::wstring wscCharname =
            (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        // Get the parameters from the user command.

        std::wstring wscClientID = GetParam(wscParam, L' ', 0);
        uint iClientIDTarget = ToInt(wscClientID);
        if (!HkIsValidClientID(iClientIDTarget) ||
            HkIsInCharSelectMenu(iClientIDTarget)) {
            PrintUserCmdText(iClientID, L"Error: Invalid client-id");
            return;
        }

        std::wstring wscTargetCharname =
            (const wchar_t*)Players.GetActiveCharacterName(iClientIDTarget);
        std::wstring wscCash = GetParam(wscParam, L' ', 1);
        std::wstring wscAnon = GetParam(wscParam, L' ', 2);
        wscCash = ReplaceStr(wscCash, L".", L"");
        wscCash = ReplaceStr(wscCash, L",", L"");
        wscCash = ReplaceStr(wscCash, L"$", L"");
        int cash = ToInt(wscCash);
        if ((!wscTargetCharname.length() || cash <= 0) ||
            (wscAnon.size() && wscAnon != L"anon")) {
            PrintUserCmdText(iClientID, L"ERR Invalid parameters");
            PrintUserCmdText(iClientID, L"/sendcash$ <id> <cash> [anon]");
            return;
        }

        bool bAnon = false;
        if (wscAnon == L"anon")
            bAnon = true;

        if (HkGetAccountByCharname(wscTargetCharname) == 0) {
            PrintUserCmdText(iClientID, L"ERR char does not exist");
            return;
        }

        int secs = 0;
        HkGetOnlineTime(wscCharname, secs);
        if (secs < SendCash::set_iMinTime) {
            PrintUserCmdText(iClientID, L"ERR insufficient time online");
            return;
        }

        // Read the current number of credits for the player
        // and check that the character has enough cash.
        int iCash = 0;
        if ((err = HkGetCash(wscCharname, iCash)) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR " + HkErrGetText(err));
            return;
        }
        if (cash < SendCash::set_iMinTransfer || cash < 0) {
            PrintUserCmdText(iClientID,
                L"ERR Transfer too small, minimum transfer " +
                ToMoneyStr(SendCash::set_iMinTransfer) +
                L" credits");
            return;
        }
        if (iCash < cash) {
            PrintUserCmdText(iClientID, L"ERR Insufficient credits");
            return;
        }

        // Prevent target ship from becoming corrupt.
        float fTargetValue = 0.0f;
        if ((err = HKGetShipValue(wscTargetCharname, fTargetValue)) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR " + HkErrGetText(err));
            return;
        }
        if ((fTargetValue + cash) > 2000000000.0f) {
            PrintUserCmdText(iClientID, L"ERR Transfer will exceed credit limit");
            return;
        }

        // Calculate the new cash
        int iExpectedCash = 0;
        if ((err = HkGetCash(wscTargetCharname, iExpectedCash)) != HKE_OK) {
            PrintUserCmdText(iClientID,
                L"ERR Get cash failed err=" + HkErrGetText(err));
            return;
        }
        iExpectedCash += cash;

        // Do an anticheat check on the receiving character first.
        uint targetClientId = HkGetClientIdFromCharname(wscTargetCharname);
        if (targetClientId != -1 && !HkIsInCharSelectMenu(targetClientId)) {
            if (HkAntiCheat(targetClientId) != HKE_OK) {
                PrintUserCmdText(iClientID, L"ERR Transfer failed");
                AddLog(
                    "NOTICE: Possible cheating when sending %s credits from %s "
                    "(%s) "
                    "to %s (%s)",
                    wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname)))
                    .c_str(),
                    wstos(wscTargetCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                    .c_str());
                return;
            }
            HkSaveChar(targetClientId);
        }

        if (targetClientId != -1) {
            if (HkIsValidClientID(ClientInfo[iClientID].iTradePartner) ||
                HkIsValidClientID(ClientInfo[targetClientId].iTradePartner)) {
                PrintUserCmdText(iClientID, L"ERR Trade window open");
                AddLog(
                    "NOTICE: Trade window open when sending %s credits from %s "
                    "(%s) "
                    "to %s (%s) %u %u",
                    wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname)))
                    .c_str(),
                    wstos(wscTargetCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                    .c_str(),
                    iClientID, targetClientId);
                return;
            }
        }

        // Remove cash from current character and save it checking that the
        // save completes before allowing the cash to be added to the target ship.
        if ((err = HkAddCash(wscCharname, 0 - cash)) != HKE_OK) {
            PrintUserCmdText(iClientID,
                L"ERR Remove cash failed err=" + HkErrGetText(err));
            return;
        }

        if (HkAntiCheat(iClientID) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR Transfer failed");
            AddLog(
                "NOTICE: Possible cheating when sending %s credits from %s (%s) to "
                "%s (%s)",
                wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname))).c_str(),
                wstos(wscTargetCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                .c_str());
            return;
        }
        HkSaveChar(iClientID);

        // Add cash to target character
        if ((err = HkAddCash(wscTargetCharname, cash)) != HKE_OK) {
            PrintUserCmdText(iClientID,
                L"ERR Add cash failed err=" + HkErrGetText(err));
            return;
        }

        targetClientId = HkGetClientIdFromCharname(wscTargetCharname);
        if (targetClientId != -1 && !HkIsInCharSelectMenu(targetClientId)) {
            if (HkAntiCheat(targetClientId) != HKE_OK) {
                PrintUserCmdText(iClientID, L"ERR Transfer failed");
                AddLog(
                    "NOTICE: Possible cheating when sending %s credits from %s "
                    "(%s) "
                    "to %s (%s)",
                    wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname)))
                    .c_str(),
                    wstos(wscTargetCharname).c_str(),
                    wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                    .c_str());
                return;
            }
            HkSaveChar(targetClientId);
        }

        // Check that receiving character has the correct ammount of cash.
        int iCurrCash;
        if ((err = HkGetCash(wscTargetCharname, iCurrCash)) != HKE_OK ||
            iCurrCash != iExpectedCash) {
            AddLog(
                "ERROR: Cash transfer error when sending %s credits from %s (%s) "
                "to "
                "%s (%s) current %s credits expected %s credits ",
                wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname))).c_str(),
                wstos(wscTargetCharname).c_str(),
                wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
                .c_str(),
                wstos(ToMoneyStr(iCurrCash)).c_str(),
                wstos(ToMoneyStr(iExpectedCash)).c_str());
            PrintUserCmdText(iClientID, L"ERR Transfer failed");
            return;
        }

        // If the target player is online then send them a message saying
        // telling them that they've received the cash.
        std::wstring msg = L"You have received " + ToMoneyStr(cash) +
            L" credits from " +
            ((bAnon) ? L"anonymous" : wscCharname);
        if (targetClientId != -1 && !HkIsInCharSelectMenu(targetClientId)) {
            PrintUserCmdText(targetClientId, L"%s", msg.c_str());
        }
        // Otherwise we assume that the character is offline so we record an entry
        // in the character's givecash.ini. When they come online we inform them
        // of the transfer. The ini is cleared when ever the character logs in.
        else {
            std::wstring msg = L"You have received " + ToMoneyStr(cash) +
                L" credits from " +
                ((bAnon) ? L"anonymous" : wscCharname);
            SendCash::LogTransfer(wscTargetCharname, msg);
        }

        AddLog("Send %s credits from %s (%s) to %s (%s)",
            wstos(ToMoneyStr(cash)).c_str(), wstos(wscCharname).c_str(),
            wstos(HkGetAccountID(HkGetAccountByCharname(wscCharname))).c_str(),
            wstos(wscTargetCharname).c_str(),
            wstos(HkGetAccountID(HkGetAccountByCharname(wscTargetCharname)))
            .c_str());

        // A friendly message explaining the transfer.
        msg = L"You have sent " + ToMoneyStr(cash) + L" credits to " +
            wscTargetCharname;
        if (bAnon)
            msg += L" anonymously";
        PrintUserCmdText(iClientID, L"%s", msg.c_str());
        return;
    }

    // Contributor TextBox
    void UserCMD_Contributor(uint iClientID, const std::wstring& wscParam) {
        FmtStr caption(0, 0);
        caption.begin_mad_lib(PopUp::iContributor_Head);
        caption.end_mad_lib();

        FmtStr message(0, 0);
        message.begin_mad_lib(PopUp::iContributor_Body);
        message.end_mad_lib();

        pub::Player::PopUpDialog(iClientID, caption, message, POPUPDIALOG_BUTTONS_CENTER_OK);
        return;
    }

    static void UserCMD_Rename(uint clientId, const std::wstring& wscParam)
    {
        const std::wstring trimmedName = Trim(wscParam);
        if (NameLimiter::IsCharacterNameAllowed(wstos(trimmedName)))
        {
            const auto result = HkRename(ARG_CLIENTID(clientId), trimmedName, false);
            if (result == HKE_CHARNAME_ALREADY_EXISTS)
                PrintUserCmdText(clientId, L"The character name is already taken. Please try another name.");
            else if (result == HKE_CHARNAME_TOO_LONG)
                PrintUserCmdText(clientId, L"The character name must not be longer than 23 characters. Please try another name.");
            else if (result == HKE_CHARNAME_TOO_SHORT)
                PrintUserCmdText(clientId, L"Please enter a name.");
            else if (result != HKE_OK)
                PrintUserCmdText(clientId, L"Something went wrong while renaming. Try again.");
        }
        else
        {
            PrintUserCmdText(clientId, L"The entered name is invalid. Please try again using the following valid characters: a-z A-Z 0-9 & ( ) [ ] { } < > - . : = _");
        }
    }

    USERCMD UserCmds[] = {
        {L"/uv", UserCmd_UV},
        {L"/sendcash", UserCMD_SendCash},
        {L"/sendcash$", UserCMD_SendCash$},
        {L"/contributor", UserCMD_Contributor},
        {L"/help", UserCmd_HELP},
        {L"/cloak", Cloak::UserCmd_CLOAK},
        {L"/c", Cloak::UserCmd_CLOAK},
        {L"/uncloak", Cloak::UserCmd_UNCLOAK},
        {L"/uc", Cloak::UserCmd_UNCLOAK},
        {L"/dock", Carrier::UserCmd_Dock},
        {L"/mark", Mark::UserCmd_Mark},
        {L"/m", Mark::UserCmd_Mark},
        {L"/groupmark", Mark::UserCmd_GroupMark},
        {L"/gm", Mark::UserCmd_GroupMark},
        {L"/unmark", Mark::UserCmd_UnMark},
        {L"/um", Mark::UserCmd_UnMark},
        {L"/groupunmark", Mark::UserCmd_UnGroupMark},
        {L"/gum", Mark::UserCmd_UnGroupMark},
        {L"/unmarkall", Mark::UserCmd_UnMarkAll},
        {L"/uma", Mark::UserCmd_UnMarkAll},
        {L"/hostile", IFF::UserCmd_Hostile},
        {L"/neutral", IFF::UserCmd_Neutral},
        {L"/friend", IFF::UserCmd_Allied},
        {L"/iff", IFF::UserCmd_Attitude},
        {L"/rename", UserCMD_Rename},
    };

    // User command processing
    bool UserCmd_Process(uint iClientID, const std::wstring& wscCmd) {
        std::wstring wscCmdLower = ToLower(wscCmd);
        for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++) {
            if (wscCmdLower.find(ToLower(UserCmds[i].wszCmd)) == 0) {
                std::wstring wscParam = L"";
                if (wscCmd.length() > wcslen(UserCmds[i].wszCmd)) {
                    if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
                        continue;
                    wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
                }
                UserCmds[i].proc(iClientID, wscParam);
                returncode = SKIPPLUGINS_NOFUNCTIONCALL;
                return true;
            }
        }
        returncode = DEFAULT_RETURNCODE;
        return false;
    }
}
