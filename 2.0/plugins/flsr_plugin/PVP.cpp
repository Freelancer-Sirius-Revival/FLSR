#include "main.h"

namespace PVP {

    std::list<Fights> ServerFightData;

    /*Commands
    /duel
    /ffa
    /ranked

    -Rankedlist
    /flsrstats
    /flstatts <player>

    */
    //Tools
    uint IsInFight(uint iClientID) 
    {
        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        uint ireturn = 0;

        for (const auto& fight : ServerFightData) {
            for (const auto& member : fight.lMembers) {
                if (member.bIsInDuel && member.wscCharname == wscCharname) {
                    ireturn = fight.iFightID;
                }

                //ConPrint (L"Member: %s\n", member.wscCharname.c_str());
            }
        }
        return ireturn;
    }

    bool IsInSameFight(uint iClientID, uint iClientID2)
    {
        std::wstring wscCharname1 = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::wstring wscCharname2 = (const wchar_t*)Players.GetActiveCharacterName(iClientID2);

        for (const auto& fight : ServerFightData) {
            bool found1 = false;
            bool found2 = false;
            for (const auto& member : fight.lMembers) {
                if (!found1 && member.bIsInDuel && member.wscCharname == wscCharname1) {
                    found1 = true;
                }
                if (!found2 && member.bIsInDuel && member.wscCharname == wscCharname2) {
                    found2 = true;
                }
                // Optional: Wenn beide IDs bereits gefunden wurden, kann die Schleife vorzeitig beendet werden, um die Leistung zu verbessern.
                if (found1 && found2) {
                    break;
                }
            }
            if (found1 && found2) {
                return true;
            }
        }
        return false;
    }

    
    PVPType GetPVPType(uint iClientID)
    {
        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        for (const auto& fight : ServerFightData) {
            for (const auto& member : fight.lMembers) {
                if (member.bIsInDuel && member.wscCharname == wscCharname) {
                    return fight.ePVPType;
                }
            }
        }
        return PVPTYPE_NONE;
    }
    
   std::list<Member> GetPVPMember(uint iClientID)
    {
        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        for (const auto& fight : ServerFightData) {
            for (const auto& member : fight.lMembers) {
                if (member.bIsInDuel && member.wscCharname == wscCharname) {
                    return fight.lMembers;
                }
            }
        }
		return std::list<PVP::Member>();
    }

   void HandleKill(uint iClientKillerID) {
       std::wstring killerCharName = (const wchar_t*)Players.GetActiveCharacterName(iClientKillerID);

       // Durchsuche die ServerFightData-Liste nach dem Fight, der den Killer enthält
       for (auto& fight : ServerFightData) {
           for (auto& member : fight.lMembers) {
               if (member.bIsInDuel && member.wscCharname == killerCharName) {
                   // Erstelle eine aktualisierte Member-Instanz
                   Member updatedMember;
                   updatedMember.wscCharname = member.wscCharname;
                   updatedMember.iKills = member.iKills;
                   updatedMember.bIsInDuel = member.bIsInDuel;

                   // Führe die gewünschten Änderungen an der Member-Instanz durch
                   updatedMember.iKills++;
                   ConPrint(L"Kills: %u\n", updatedMember.iKills);

                   // Suche manuell nach dem entsprechenden Element und aktualisiere es
                   for (auto it = fight.lMembers.begin(); it != fight.lMembers.end(); ++it) {
                       if (it->wscCharname == member.wscCharname) {
                           *it = updatedMember; // Aktualisiere das Element in der Liste
                           break;
                       }
                   }

                   // Optional: Hier kannst du weitere Aktionen ausführen, die du für notwendig hältst
                   // ...

                   break; // Breche die Schleife ab, da der Killer gefunden wurde
               }
           }
       }

       ConPrint(L"TESTDEATH-HandleKill\n");
   }




   uint GetKills(uint iClientID)
   {
	   std::wstring wscCharnameClient = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
       //Debug Print Charname
       ConPrint(L"CharnameKillerWanted: %s\n", wscCharnameClient.c_str());
       for (const auto& fight : ServerFightData) {
           for (const auto& member : fight.lMembers) {
               //Debug Print Charname
               ConPrint(L"Charname: %s\n", member.wscCharname.c_str());
               //Debug Print kills
               ConPrint(L"Kills: %u\n", member.iKills);
               if (member.bIsInDuel && member.wscCharname == wscCharnameClient) {
                   ConPrint(L"CharnameKiller: %s\n", member.wscCharname.c_str());
                   ConPrint(L"KillsKiller: %u\n", member.iKills);

				   return member.iKills;
			   }
		   }
	   }
	   return 0;
   }
    
    void AcceptFight(Fights& fight, uint iClientID)
    {
        // Check if player is already in the fight
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
        bool bFound = false;
        for (auto& member : fight.lMembers) {
            if (member.wscCharname == wscCharname) {
                member.bIsInDuel = true;
                bFound = true;
                break;
            }
        }

        // If player not found, add to the fight
        if (!bFound) {
            Member member;
            member.wscCharname = wscCharname;
            member.bIsInDuel = true;
            fight.lMembers.push_back(member);
        }

        // Update the fight in the ServerFightData
        for (auto& f : ServerFightData) {
            if (f.iFightID == fight.iFightID) {
                f = fight;
                break;
            }
        }

        // Notify all members that the fight has started, except the joining player
        std::wstring wscMsg = L" has joined the fight!";
        std::wstring wscJoinerName = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        for (auto& member : fight.lMembers) {
            uint MemberClientID = HkGetClientIdFromCharname(member.wscCharname);
            if (MemberClientID != -1) {
                std::wstring wscNotifyMsg = wscJoinerName + wscMsg;
                if (MemberClientID == iClientID)
                {
                    PrintUserCmdText(MemberClientID, L"You joined the fight!");
                }
                else
                {
                    PrintUserCmdText(MemberClientID, wscNotifyMsg.c_str());
                }
            }
        }

    }


    //Commands
    void CmdDuel(uint iClientID, const std::wstring& wscParam)
    {
        // The last error.
        HK_ERROR err;

        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        //Init variables
        bool bBet = false;
        uint iFights = 5;
        int cash = 0;

        // Überprüfe ob Spieler im Space ist
        uint iShip;
        pub::Player::GetShip(iClientID, iShip);
        if (!iShip) 
        {
            PrintUserCmdText(iClientID, L"Please undock!");
            return;
        }

        //Hat der Spieler einen aktiven Fight?
        uint iFightID = IsInFight(iClientID);
        if (iFightID != 0)
        {
            PrintUserCmdText(iClientID, L"You are already in a fight!");
            return;
        }

        // Überprüfe auf Target
        uint iTargetShip;
        pub::SpaceObj::GetTarget(iShip, iTargetShip);
        if (!iTargetShip) 
        {
            PrintUserCmdText(iClientID, L"Please select a target!");
            return;
        }

        // Überprüfe ob Target ein Spieler ist
        uint iTargetClientID = HkGetClientIDByShip(iTargetShip);
        if (!iTargetClientID) 
        {
            PrintUserCmdText(iClientID, L"Please select a player!");
            return;
        }

        std::wstring wscTargetCharname = (const wchar_t*)Players.GetActiveCharacterName(iTargetClientID);

        // Überprüfe ob Target bereits in einem Fight ist
        uint iFightIDTarget = IsInFight(iTargetClientID);
        if (iFightIDTarget != 0)
        {
            PrintUserCmdText(iClientID, L"Target is already in a fight!");
            return;
        }
        
        //Fights
        std::wstring wscFights = GetParam(wscParam, L' ', 0);
        if (wscFights != L"") 
        {
            iFights = stoi(wscFights);

        }

        //Gamble
        std::wstring wscCash = GetParam(wscParam, L' ', 1);
        if (wscCash != L"")
        {
            wscCash = ReplaceStr(wscCash, L".", L"");
            wscCash = ReplaceStr(wscCash, L",", L"");
            wscCash = ReplaceStr(wscCash, L"$", L"");
            cash = ToInt(wscCash);
            if (cash <= 0) 
            {
                PrintUserCmdText(iClientID, L"ERR Invalid parameters");
                PrintUserCmdText(iClientID, L"/duel <cash>");
                return;
            }

            // Read the current number of credits for the player
            int iCash = 0;
            if ((err = HkGetCash(wscCharname, iCash)) != HKE_OK) 
            {
                PrintUserCmdText(iClientID, L"ERR " + HkErrGetText(err));
                return;
            }
            if (iCash < cash) 
            {
                PrintUserCmdText(iClientID, L"ERR Insufficient credits");
                return;
            }

            bBet = true;
        }


        //Starte Fight
        PVP::Fights newFight;
        
        //Füge die Mitglieder hinzu
        newFight.lMembers.push_back({ wscCharname, 0, true }); //Herausforderer
        newFight.lMembers.push_back({ wscTargetCharname, 0, false }); //Herausgeforderter

        //Setze den Gamble-Wert
        if (bBet) {
            newFight.iGambleValue = cash;
        }

        // Setze den PVP-Typ
        newFight.ePVPType = PVP::PVPTYPE_DUEL;

		// Setze die Anzahl der Fights
		newFight.iFights = iFights;
		newFight.iFightsRemaining = iFights;

        // Füge den neuen Kampf zur Liste hinzu
        ServerFightData.push_back(newFight);

        // Gib beiden Spielern ein Feedback
        std::wstring wscMessage = L"You have been challenged to a duel by " + wscCharname + L"!";
        PrintUserCmdText(iTargetClientID, wscMessage);
        wscMessage = L"You have challenged " + wscTargetCharname + L" to a duel!";
        PrintUserCmdText(iClientID, wscMessage);
        
    }

    void CmdAcceptPVP(uint iClientID, const std::wstring& wscParam)
    {
        // Check if player is in a fight
        uint iFightID = IsInFight(iClientID);
        if (iFightID != 0)
        {
            PrintUserCmdText(iClientID, L"You are already in a fight!");
            return;
        }

        // Check if there is a PvP invitation to accept
        auto& fights = ServerFightData;
        auto iter = std::find_if(fights.begin(), fights.end(), [&](const PVP::Fights& f) {
            std::wstring activeCharacterName = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        for (const auto& member : f.lMembers) {
            if (member.wscCharname == activeCharacterName) {
                if (!member.bIsInDuel)
                {
                    return true;
                }
            }
        }

        return false;
            });

        if (iter != fights.end()) {
            // Accept the fight invitation
            AcceptFight(*iter, iClientID);
        }
        else {
            PrintUserCmdText(iClientID, L"There are no PvP invitations to accept!");
        }
    }

    //Hooks
    void CheckDisConnect(uint iClientID, DisconnectReason reason)
    {
        std::wstring wscCharname = L"";

        if (!Tools::isValidPlayer(iClientID, false))
            return;
        
        //Check if there are fights
        if (ServerFightData.size() == 0)
			return;

        // Get the character name of the player if Player is not in CharSelect
        if (!HkIsInCharSelectMenu(iClientID))
        {
            wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
        }

        // Check if player was in a fight
        for (auto& fight : ServerFightData) {
            auto iter = std::find_if(fight.lMembers.begin(), fight.lMembers.end(), [&](const Member& m) {
                return m.wscCharname == wscCharname;
                });

            // Player is in the fight
            if (iter != fight.lMembers.end()) {
                // Remove player from the fight
                fight.lMembers.erase(iter);

                // If only one player is left, end the fight and declare the remaining player as the winner
                if (fight.lMembers.size() == 1) {
                    Member& winner = fight.lMembers.front();
                    std::wstring wscWinnerMsg = winner.wscCharname + L" has won the fight";
					if (reason == DisconnectReason::DISCONNECTED) {
                        wscWinnerMsg += L" through disconnect of the last opponent!";
                    }
                    else if (reason == DisconnectReason::CHARSWITCH) {
                        wscWinnerMsg += L" through character switch of the last opponent!";
                    }
                    for (auto& member : fight.lMembers) {
                        ConPrint(L"Member: " + member.wscCharname + L"\n");
                        uint MemberClientID = HkGetClientIdFromCharname(member.wscCharname);
                        if (MemberClientID != -1) {
                            PrintUserCmdText(MemberClientID, wscWinnerMsg.c_str());
                        }
                    }

                    // Call the FightEnd function to clean up the fight
                    //FightEnd(fight.iFightID);
                }
                else {
                    // Update the fight in the ServerFightData
                    auto iter2 = std::find_if(ServerFightData.begin(), ServerFightData.end(), [&](const Fights& f) {
                        return f.iFightID == fight.iFightID;
                        });

                    if (iter2 != ServerFightData.end()) {
                        *iter2 = fight;
                    }
                }

                break;
            }
        }
    }

    void CheckDied(uint iClientID, uint iClientKillerID, Tools::eDeathTypes DeathType)
    {
		// The iClient must be a valid player
		if (!Tools::isValidPlayer(iClientID, false))
			return;
        
        // Check if player is in a fight
        uint iFightIDClient = IsInFight(iClientID);
        if (iFightIDClient == 0)
        {
			//No Death in a Fight
            return;
        }
        
        //Get PVPType
        PVPType ePVPType = GetPVPType(iClientID);
        
		//Check for DeathType - SUICIDE Kill
        if (DeathType == Tools::SUICIDE && ePVPType != PVPTYPE_RANKED)
        {

        }

        //Check for DeathType - KILLEDHIMSELF Kill
        if (DeathType == Tools::KILLEDHIMSELF && ePVPType != PVPTYPE_RANKED)
        {

        }

        //Debug
        ConPrint(L"TESTDEATH\n");

        //Check for DeathType - PVP Kill
        if (DeathType == Tools::PVP)
        {
            ConPrint(L"TESTDEATH1\n");

            // The iClientKillerID must be a valid player
            if (!Tools::isValidPlayer(iClientKillerID, false))
                return;

           ConPrint(L"TESTDEATH1.1\n");



            if (!IsInSameFight(iFightIDClient, iClientKillerID))
            {
                //No Death in a Fight
                ConPrint(L"NO DEATH in a FIGHT\n");
                return;
            }


            ConPrint(L"TESTDEATH1.2\n");

            //HandleKill
            //Debug
            HandleKill(iClientKillerID);
            ConPrint(L"Kill handeld\n");

            //Show kills of Client
            uint iKills = GetKills(iClientKillerID);
            ConPrint (L"NewKills: " + std::to_wstring(iKills) + L"\n");

        }

    }

}