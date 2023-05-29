#include "Main.h"

namespace Timers {
    void TimerUpdate1000ms() {

        // count players
        int iPlayers = 0;
        std::string playeronlinefile = PLAYERONLINE_FILE;

        try {

            /**************************************************************************************************************
            PlayerOnlineFile
            **************************************************************************************************************/

            remove(playeronlinefile.c_str()); // delete file

            iPlayers = 0;
            struct PlayerData *pPD = 0;
            while (pPD = Players.traverse_active(pPD)) {
                
                uint ipClientID = HkGetClientIdFromPD(pPD);

                HK_ERROR err;

                if (ipClientID == -1 || HkIsInCharSelectMenu(ipClientID))
                    continue;
                std::wstring wscCharFileName;
                if ((err = HkGetCharFileName(ARG_CLIENTID(ipClientID), wscCharFileName)) != HKE_OK) {
                    continue;
                }


                std::wstring wscCharname =
                    (wchar_t *)Players.GetActiveCharacterName(ipClientID);
                std::string scCharname = wstos(wscCharname);

                // Write Charname to Ini
                IniWrite(playeronlinefile, "Data", "Player" + std::to_string(iPlayers), scCharname);

                iPlayers++;
            }
			


            /**************************************************************************************************************
            Discord Data - Send
            **************************************************************************************************************/

            // Online Players to ini
            IniWrite(playeronlinefile, "Data", "PlayerOnline", std::to_string(iPlayers));

            // Max Player to ini
            IniWrite(playeronlinefile, "Data", "MaxPlayers", std::to_string(Players.GetMaxPlayerCount()));

            /**************************************************************************************************************
            Discord Data - Get
            **************************************************************************************************************/

            std::string ChatMessage;
            std::string ChatFilePath = DISCORD_CHAT_FILE;
            std::ifstream Chat(ChatFilePath);
            bool copen = false;
            if (Chat.is_open()) {

                while (std::getline(Chat, ChatMessage)) {
                    if (ChatMessage != "") {

                        std::string delimiter = "$%&";
                        std::string Charname =
                            ChatMessage.substr(0, ChatMessage.find(delimiter));
                        int pos2 = ChatMessage.find(delimiter) + 4;
                        ChatMessage.erase(0, pos2);
                        Chat::HkSendUChat(stows(Charname), stows(ChatMessage));
                    }
                }
                copen = true;
                Chat.close();
            }
            if (copen) {
                remove(ChatFilePath.c_str());
            }

        } catch (...) {
        }
    }

    TIMER Timers[] = {
       {Cloak::CloakInstallTimer2000ms, 2000, 0},
	   {Cloak::WarmUpCloakTimer1000ms, 1000, 0},
       {Cloak::DoCloakingTimer250ms, 250, 0},
       {Cloak::UpdateShipEnergyTimer, 1500, 0},
       {TimerUpdate1000ms, 1000, 0},
       {Docking::DockRequest3000ms, 3000, 0},
       {PathSelection::ModUnlawfulChar500ms, 500, 0},
       {Tools::CharSelectMenu, 2000, 0},

    };

    int __stdcall Update() {
        returncode = DEFAULT_RETURNCODE;

        // call timers
        for (uint i = 0; (i < sizeof(Timers) / sizeof(TIMER)); i++) {
            if ((timeInMS() - Timers[i].tmLastCall) >= Timers[i].tmIntervallMS) {
              
  
                    Timers[i].tmLastCall = timeInMS();
                    Timers[i].proc();
 
                
   
            }
        }

        return 0; // it doesnt matter what we return here since we have set the
                  // return code to "DEFAULT_RETURNCODE", so FLHook will just ignore
                  // it
    }


}