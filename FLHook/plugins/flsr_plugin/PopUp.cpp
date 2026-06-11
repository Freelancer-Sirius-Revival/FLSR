#include "Main.h"

namespace PopUp {
	// Welcome Message - First Char on ID
	uint iWMsg_Head = 524289;
	uint iWMsg_Body = 524290;

	void WelcomeBox(uint iClientID) 
	{
        std::wstring wscCharnameWelcomePopUp = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        CAccount *acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        const std::string FLHOOKUSER_FILE = "\\flhookuser.ini";
        std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;
        std::string sWelcomePopupSend = IniGetS(scUserFile, "Settings", "WelcomePopup", "no");
        if (sWelcomePopupSend == "no") {

            // SystemCheck
            uint iSysID;
            pub::Player::GetSystem(iClientID, iSysID);
            char szSystemname[1024] = "";
            pub::GetSystemNickname(szSystemname, sizeof(szSystemname), iSysID);
            if (stows(szSystemname) == L"start") 
            {
                FmtStr WelcomeCaption(0, 0);
                WelcomeCaption.begin_mad_lib(PopUp::iWMsg_Head);
                WelcomeCaption.end_mad_lib();

                FmtStr WelcomeMessage(0, 0);
                WelcomeMessage.begin_mad_lib(PopUp::iWMsg_Body);
                WelcomeMessage.end_mad_lib();

                pub::Player::PopUpDialog(iClientID, WelcomeCaption, WelcomeMessage, PopupDialogButton::CENTER_OK);
            }
            IniWrite(scUserFile, "Settings", "WelcomePopup", "yes");
        }
	}
}