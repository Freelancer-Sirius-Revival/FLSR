#include "Main.h"

namespace PopUp {
	// Contributor Box - Command
	uint iContributor_Head = 520000;
	uint iContributor_Body = 520001;

	// Welcome Message - First Char on ID
	uint iWMsg_Head = 524289;
	uint iWMsg_Body = 524290;

    std::map<std::wstring, PopUpBox> mPopUpBox;

	void WelcomeBox(uint iClientID) 
	{
        if (Modules::GetModuleState("WelcomeMSG")) {
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

                    pub::Player::PopUpDialog(iClientID, WelcomeCaption, WelcomeMessage, POPUPDIALOG_BUTTONS_CENTER_OK);
                }
                IniWrite(scUserFile, "Settings", "WelcomePopup", "yes");
            }
        }
	}

    void OpenPopUp(uint iClientID)
    {
        //Get Data
        std::wstring wscCharFileName;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);

        //Get PopUpBox
        PopUpBox NewPopUpBox = PopUp::mPopUpBox[wscCharFileName];
        
        FmtStr Caption(0, 0);
		Caption.begin_mad_lib(NewPopUpBox.iHead);
        Caption.end_mad_lib();

        FmtStr Message(0, 0);
        Message.begin_mad_lib(NewPopUpBox.iBody);
        Message.end_mad_lib();

        pub::Player::PopUpDialog(iClientID, Caption, Message, NewPopUpBox.iButton);
        




    }
    
    void HandleButtonClick(uint iClientID, uint buttonClicked)
    {
        //Not used
        return;

        
        //Get Data
        std::wstring wscCharFileName;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);

        //Get PopUpBox
        PopUpBox NewPopUpBox = PopUp::mPopUpBox[wscCharFileName];

       //Check Type
        switch (NewPopUpBox.iType) {
        //case Help
        case PopUpType_Help:

            //Check if we need to show a new PopUp
            if (NewPopUpBox.iPage < NewPopUpBox.iMaxPage)
            {
				//Show Page 2 if POPUPDIALOG_BUTTONS_RIGHT_LATER is clicked
                if (NewPopUpBox.iPage == 1 && buttonClicked == POPUPDIALOG_BUTTONS_RIGHT_LATER)
                {
                    //Count iPage
                    uint iPage = NewPopUpBox.iPage;
                    iPage++;

                    //Create Popup struct
                    PopUp::PopUpBox NewPopUpBoxNext;
                    NewPopUpBoxNext.iClientID = iClientID;
                    NewPopUpBoxNext.iHead = PopUp::iWMsg_Head;
                    NewPopUpBoxNext.iBody = PopUp::iWMsg_Body;
                    NewPopUpBoxNext.iPage = iPage;
                    NewPopUpBoxNext.iMaxPage = NewPopUpBox.iMaxPage;
                    NewPopUpBoxNext.iButton = POPUPDIALOG_BUTTONS_CENTER_OK;

                    //Setup New Popup
                    PopUp::mPopUpBox[wscCharFileName] = NewPopUpBoxNext;

                    //Show Page
                    OpenPopUp(iClientID);
                }
            }
            break;
        }
        
    }

}