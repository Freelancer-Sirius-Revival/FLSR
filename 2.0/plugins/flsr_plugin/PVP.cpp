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

    void LoadPVP()
    {
        // Calculate Ranking
        CalcRanking("DuelRanking");
        CalcRanking("FFARanking");
        CalcRanking("PVPRanking");
        CalcRanking("PVERanking");

        //Clear FightInfo
        ClearFightInfo();

    }

    std::wstring GetWStringFromPVPTYPE(PVPType pvpType)
    {
        switch (pvpType)
        {
        case PVPTYPE_DUEL:
            return L"Duel";
        case PVPTYPE_FFA:
            return L"FFA";
        case PVPTYPE_RANKED:
            return L"Ranked";
        default:
            return L"Unknown";
        }
    }


    uint IsInFight(uint iClientID, bool bSkipFightCheck = false)
    {
        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        for (const auto& fight : ServerFightData) {
            for (const auto& member : fight.lMembers) {
                if (!bSkipFightCheck && !member.bIsInFight)
                    continue;

                if (member.wscCharname == wscCharname) {
                    return fight.iFightID;
                }
            }
        }

        return 0;
    }


    PVPType GetActiveFightPVPType(uint iFightID)
    {
        for (const auto& fight : ServerFightData)
        {
            if (fight.iFightID == iFightID)
            {
                return fight.ePVPType;
            }
        }
        return PVPTYPE_NONE; // Falls kein aktiver Fight mit der gegebenen iFightID gefunden wurde
    }


    bool IsInSameFight(uint iClientID, uint iClientID2)
    {
        std::wstring wscCharname1 = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::wstring wscCharname2 = (const wchar_t*)Players.GetActiveCharacterName(iClientID2);

        for (const auto& fight : ServerFightData) {
            bool found1 = false;
            bool found2 = false;
            for (const auto& member : fight.lMembers) {
                if (!found1 && member.bIsInFight && member.wscCharname == wscCharname1) {
                    found1 = true;
                }
                if (!found2 && member.bIsInFight && member.wscCharname == wscCharname2) {
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

    uint GetFightIDByCharname(const std::wstring& wscCharname)
    {
        for (const auto& fight : ServerFightData)
        {
            for (const auto& member : fight.lMembers)
            {
                if (member.wscCharname == wscCharname)
                {
                    return fight.iFightID;
                }
            }
        }
        return 0; // Falls keine entsprechende FightID gefunden wurde
    }


    
    PVPType GetPVPType(uint iClientID)
    {
        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        for (const auto& fight : ServerFightData) {
            for (const auto& member : fight.lMembers) {
                if (member.bIsInFight && member.wscCharname == wscCharname) {
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
                if (member.bIsInFight && member.wscCharname == wscCharname) {
                    return fight.lMembers;
                }
            }
        }
		return std::list<PVP::Member>();
    }

   void HandleKill(uint iClientKillerID, PVPType ePVPType) {
       std::wstring killerCharName = (const wchar_t*)Players.GetActiveCharacterName(iClientKillerID);

       // Durchsuche die ServerFightData-Liste nach dem Fight, der den Killer enthält
       for (auto& fight : ServerFightData) {
           for (auto& member : fight.lMembers) {
               if (member.bIsInFight && member.wscCharname == killerCharName) {
                   // Erstelle eine aktualisierte Member-Instanz
                   Member updatedMember;
                   updatedMember.wscCharname = member.wscCharname;
                   updatedMember.wscCharFilename = member.wscCharFilename;
                   updatedMember.iKills = member.iKills;
                   updatedMember.bIsInFight = member.bIsInFight;

                   // Führe die gewünschten Änderungen an der Member-Instanz durch
                   updatedMember.iKills++;
                   //ConPrint(L"Kills: %u\n", updatedMember.iKills);

                   // Suche manuell nach dem entsprechenden Element und aktualisiere es
                   for (auto it = fight.lMembers.begin(); it != fight.lMembers.end(); ++it) {
                       if (it->wscCharname == member.wscCharname) {
                           *it = updatedMember; // Aktualisiere das Element in der Liste

                           // Verringere die Anzahl der übrigen Fights um eins und aktualisiere ServerFightData
                           // Nicht für FFA
                           if (ePVPType != PVPTYPE_FFA)
                            fight.iFightsRemaining--;

                           //ConPrint(L"iFightsRemaining: %u\n", fight.iFightsRemaining);
                           if (ePVPType == PVPTYPE_DUEL)
                           {
                               UpdateDuelRanking(HkGetClientIdFromCharname(member.wscCharname), true); // Aktualisiere die Rangliste
                           }
                           if (ePVPType == PVPTYPE_FFA)
                           {
                               UpdateFFARanking(HkGetClientIdFromCharname(member.wscCharname), true); // Aktualisiere die Rangliste
                           }

                           break;
                       }
                   }               


                   break; // Breche die Schleife ab, da der Killer gefunden wurde
               }
           }
       }

       //ConPrint(L"TESTDEATH-HandleKill\n");
   }

   void CheckLastRound()
   {
       for (auto it = ServerFightData.begin(); it != ServerFightData.end(); ++it)
       {

           if (it->iFightsRemaining == 1)
           {
               // Notify all members of the fight that it's the last round
               for (const auto& member : it->lMembers)
               {
                   std::wstring message = L"This is the last round of the fight. Good luck, " + member.wscCharname + L"!";
                   PrintUserCmdText(HkGetClientIdFromCharname(member.wscCharname), message);
               }
               return;
           }
           else if (it->iFightsRemaining == 0)
           {
               // Determine the winner based on the highest number of kills
               std::wstring winner;
               int highestKills = 0;
               for (const auto& member : it->lMembers)
               {
                   uint kills = GetKills(HkGetClientIdFromCharname(member.wscCharname)); // Get the kills for the member
                   //Print Kills + Charname
                   //ConPrint(L"Kills: %u ", kills);
                   //ConPrint(L"Charname: %s\n", member.wscCharname.c_str());                  
                   if (kills > highestKills)
                   {
                       highestKills = kills;
                       winner = member.wscCharname;
                   }
               }

               // Notify all members of the fight that the fight has ended and declare the winner
               for (const auto& member : it->lMembers)
               {
                   std::wstring message;
                   if (member.wscCharname == winner)
                       message = L"The fight has ended. Congratulations, " + member.wscCharname + L", you are the winner with the most kills!";
                   else
                       message = L"The fight has ended. Thank you for participating, " + member.wscCharname + L"! The winner is " + winner + L" with the most kills.";

                   PrintUserCmdText(HkGetClientIdFromCharname(member.wscCharname), message);
                   
                   RemoveCharFromFightInfo(wstos(member.wscCharFilename));
               }

               // Remove the fight from ServerFightData
               it = ServerFightData.erase(it);
               if (it == ServerFightData.end())
                   return; // Exit the loop if all fights have been processed
           }
       }
   }



   uint GetKills(uint iClientID)
   {
	   std::wstring wscCharnameClient = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
       //Debug Print Charname
       //ConPrint(L"CharnameKillerWanted: %s\n", wscCharnameClient.c_str());
       for (const auto& fight : ServerFightData) {
           for (const auto& member : fight.lMembers) {
               //Debug Print Charname
               //ConPrint(L"Charname: %s\n", member.wscCharname.c_str());
               //Debug Print kills
               //ConPrint(L"Kills: %u\n", member.iKills);
               if (member.bIsInFight && member.wscCharname == wscCharnameClient) {
                   //ConPrint(L"CharnameKiller: %s\n", member.wscCharname.c_str());
                   //ConPrint(L"KillsKiller: %u\n", member.iKills);

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
        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);

        bool bFound = false;
        for (auto& member : fight.lMembers) {
            if (member.wscCharname == wscCharname) {
                member.iClientID = iClientID;
                member.bIsInFight = true;
                member.wscCharFilename = wscCharFilename;
                bFound = true;
                break;
            }
        }

        // If player not found, add to the fight
        if (!bFound) {
            Member member;
            member.iClientID = iClientID;
            member.wscCharname = wscCharname;
            member.wscCharFilename = wscCharFilename;
            member.bIsInFight = true;
            fight.lMembers.push_back(member);
        }

        // Update the fight in the ServerFightData
        for (auto& f : ServerFightData) {
            if (f.iFightID == fight.iFightID) {
                f = fight;
                break;
            }
        }

        // Update FightInfo
        WriteFightInfoToCFG(fight.iFightID, wstos(wscCharFilename));


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

    void CmdFight(uint iClientID, const std::wstring& wscParam, PVP::PVPType ePVPType)
    {
        // The last error.
        HK_ERROR err;

        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        // Get the current charfilename
        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);

        //Init variables
        uint iFights = 5;        

        // Überprüfe ob Spieler im Space ist
        uint iShip;
        pub::Player::GetShip(iClientID, iShip);
        if (!iShip) 
        {
            PrintUserCmdText(iClientID, L"Please undock!");
            return;
        }

        //Hat der Spieler einen Fight?
        uint iFightID = IsInFight(iClientID);
        if (iFightID != 0)
        {
            PrintUserCmdText(iClientID, L"You are already in a fight!");
            return;
        }

        iFightID = IsInFight(iClientID, true);
        if (iFightID != 0)
        {
            PrintUserCmdText(iClientID, L"You have been invited to a fight. Use /pvpclear to decline the invitation.");
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

        // Get Target Charname
        std::wstring wscTargetCharname = (const wchar_t*)Players.GetActiveCharacterName(iTargetClientID);
        // Get Target Charfilename
        std::wstring wscTargetCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iTargetClientID), wscTargetCharFilename);

        // Überprüfe ob Target bereits in einem Fight ist
        uint iFightIDTarget = IsInFight(iTargetClientID);
        if (iFightIDTarget != 0)
        {
            PVPType activeFightPVPType = GetActiveFightPVPType(iFightIDTarget);
            if (activeFightPVPType == PVPTYPE_DUEL || activeFightPVPType == PVPTYPE_RANKED || ePVPType == PVPTYPE_DUEL || ePVPType == PVPTYPE_FFA)
            {
                PrintUserCmdText(iClientID, L"Target is already in a " + GetWStringFromPVPTYPE(activeFightPVPType) + L" fight!");
                return;
            }

        }

        
        //Fights
        std::wstring wscFights = GetParam(wscParam, L' ', 0);
        if (wscFights != L"") 
        {
            iFights = stoi(wscFights);

        }

        //Starte Fight
        PVP::Fights newFight;
        
        //Füge die Mitglieder hinzu
        newFight.lMembers.push_back({iClientID, wscCharname, wscCharFilename, 0, true }); //Herausforderer
        newFight.lMembers.push_back({iTargetClientID, wscTargetCharname, wscTargetCharFilename, 0, false }); //Herausgeforderter

        // Setze den PVP-Typ
        newFight.ePVPType = ePVPType;

		// Setze die Anzahl der Fights
		newFight.iFights = iFights;
		newFight.iFightsRemaining = iFights;

        // Füge den neuen Kampf zur Liste hinzu
        ServerFightData.push_back(newFight);

        // Füge den Herausforderer in die FightInfo hinzu
        WriteFightInfoToCFG(GetFightIDByCharname(wscCharname), wstos(wscCharFilename));

        // Gib beiden Spielern ein Feedback
        std::wstring wscMessageTarget;
        std::wstring wscMessageClient;

        wscMessageTarget = L"You have been challenged to a " + GetWStringFromPVPTYPE(ePVPType) + L" by " + wscCharname + L"!";
        wscMessageClient = L"You have challenged " + wscTargetCharname + L" to a " + GetWStringFromPVPTYPE(ePVPType) + L"!";
 
        PrintUserCmdText(iTargetClientID, wscMessageTarget);
        PrintUserCmdText(iClientID, wscMessageClient);
        
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
                if (!member.bIsInFight)
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

    void CheckDisConnect(uint iClientID, DisconnectReason reason)
    {
        if (!Tools::isValidPlayer(iClientID, false))
            return;

        if (!HkIsValidClientID(iClientID)) {
            return;
        }

        //Check if there are fights
        if (ServerFightData.size() == 0)
            return;

        // Check if player was in a fight
        for (auto& fight : ServerFightData) {
            bool playerFound = false;
            for (auto it = fight.lMembers.begin(); it != fight.lMembers.end(); ++it) {
                if (it->iClientID == iClientID) {
                    // Player is in the fight
                    playerFound = true;
                    //Remove player from FightInfo
                    RemoveCharFromFightInfo(wstos(it->wscCharFilename));
                    // Remove player from the fight
                    fight.lMembers.erase(it);



                    // If only one player is left, end the fight and declare the remaining player as the winner
                    if (fight.lMembers.size() == 1) {
                        std::wstring wscWinnerMsg = fight.lMembers.front().wscCharname + L" has won the fight";
                        if (reason == DisconnectReason::DISCONNECTED) {
                            wscWinnerMsg += L" through disconnect of the last opponent!";
                        }
                        else if (reason == DisconnectReason::CHARSWITCH) {
                            wscWinnerMsg += L" through character switch of the last opponent!";
                        }
                       
                         PrintUserCmdText(fight.lMembers.front().iClientID, wscWinnerMsg.c_str());

                         RemoveCharFromFightInfo(wstos(fight.lMembers.front().wscCharFilename));//OMG iam so stupid
                         RemovePlayerFromFight(fight.iFightID, fight.lMembers.front().wscCharname);

                        

                        // Call the FightEnd function to clean up the fight
                        //FightEnd(fight.iFightID);
                    }

                    // Update the fight in the ServerFightData
                    for (auto& f : ServerFightData) {
                        if (f.iFightID == fight.iFightID) {
                            f = fight;
                            break;
                        }
                    }
                    


                    break;
                }
            }

            if (playerFound)
                break;
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
        //ConPrint(L"TESTDEATH\n");

        //Check for DeathType - PVP Kill
        if (DeathType == Tools::PVP)
        {
            //ConPrint(L"TESTDEATH1\n");

            // The iClientKillerID must be a valid player
            if (!Tools::isValidPlayer(iClientKillerID, false))
                return;

           //ConPrint(L"TESTDEATH1.1\n");



            if (!IsInSameFight(iFightIDClient, iClientKillerID))
            {
                //No Death in a Fight
                //ConPrint(L"NO DEATH in a FIGHT\n");
                return;
            }

            if (ePVPType == PVPTYPE_DUEL)
            {
                HandleKill(iClientKillerID, ePVPType);

                //CheckLastRound
                CheckLastRound();

                //Update Killed Ranking
                UpdateDuelRanking(iClientID, false);
            }
            else if (ePVPType == PVPTYPE_FFA)
            {
                ConPrint(L"FFA DEATH\n");
                HandleKill(iClientKillerID, ePVPType);

                //CheckLastRound
                CheckLastRound();

                //Update Killed Ranking
                UpdateFFARanking(iClientID, false);
            }

        }
    }

    void UpdatePVPRanking(uint iClientID, bool bKills)
    {
        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        try
        {
            // Open a database file with UTF-8 encoding
            SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

            // Check if the entry already exists in the table
            SQLite::Statement queryExists(db, "SELECT * FROM PVPRanking WHERE Charfile = ?");
            queryExists.bind(1, wstos(wscCharFilename));
            bool entryExists = queryExists.executeStep();

            if (entryExists)
            {
                // If the entry exists, update the Kills and Deaths values
                std::string columnName = (bKills ? "Kills" : "Deaths");
                SQLite::Statement queryUpdate(db, "UPDATE PVPRanking SET " + columnName + " = " + columnName + " + 1, Charname = ? WHERE Charfile = ?");
                queryUpdate.bind(1, wstos(wscCharname));
                queryUpdate.bind(2, wstos(wscCharFilename));
                queryUpdate.exec();
            }
            else
            {
                // If the entry doesn't exist, insert a new row with the default value of 1 for Kills and Deaths
                SQLite::Statement queryInsert(db, "INSERT INTO PVPRanking (Charname, Kills, Deaths, Charfile) VALUES (?, ?, ?, ?)");
                queryInsert.bind(1, wstos(wscCharname));
                queryInsert.bind(2, bKills ? 1 : 0);
                queryInsert.bind(3, bKills ? 0 : 1);
                queryInsert.bind(4, wstos(wscCharFilename));
                queryInsert.exec();
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }

    void UpdatePVERanking(uint iClientID, bool bKills)
    {
        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        try
        {
            // Open a database file with UTF-8 encoding
            SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

            // Check if the entry already exists in the table
            SQLite::Statement queryExists(db, "SELECT * FROM PVERanking WHERE Charfile = ?");
            queryExists.bind(1, wstos(wscCharFilename));
            bool entryExists = queryExists.executeStep();

            if (entryExists)
            {
                // If the entry exists, update the Kills and Deaths values
                std::string columnName = (bKills ? "Kills" : "Deaths");
                SQLite::Statement queryUpdate(db, "UPDATE PVERanking SET " + columnName + " = " + columnName + " + 1, Charname = ? WHERE Charfile = ?");
                queryUpdate.bind(1, wstos(wscCharname));
                queryUpdate.bind(2, wstos(wscCharFilename));
                queryUpdate.exec();
            }
            else
            {
                // If the entry doesn't exist, insert a new row with the default value of 1 for Kills and Deaths
                SQLite::Statement queryInsert(db, "INSERT INTO PVERanking (Charname, Kills, Deaths, Charfile) VALUES (?, ?, ?, ?)");
                queryInsert.bind(1, wstos(wscCharname));
                queryInsert.bind(2, bKills ? 1 : 0);
                queryInsert.bind(3, bKills ? 0 : 1);
                queryInsert.bind(4, wstos(wscCharFilename));
                queryInsert.exec();
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }

    void UpdateDuelRanking(uint iClientID, bool bKills)
    {
        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        try
        {
            // Open a database file with UTF-8 encoding
            SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

            // Check if the entry already exists in the table
            SQLite::Statement queryExists(db, "SELECT * FROM DuelRanking WHERE Charfile = ?");
            queryExists.bind(1, wstos(wscCharFilename));
            bool entryExists = queryExists.executeStep();

            if (entryExists)
            {
                // If the entry exists, update the Kills and Deaths values
                std::string columnName = (bKills ? "Kills" : "Deaths");
                SQLite::Statement queryUpdate(db, "UPDATE DuelRanking SET " + columnName + " = " + columnName + " + 1, Charname = ? WHERE Charfile = ?");
                queryUpdate.bind(1, wstos(wscCharname));
                queryUpdate.bind(2, wstos(wscCharFilename));
                queryUpdate.exec();
            }
            else
            {
                // If the entry doesn't exist, insert a new row with the default value of 1 for Kills and Deaths
                SQLite::Statement queryInsert(db, "INSERT INTO DuelRanking (Charname, Kills, Deaths, Charfile) VALUES (?, ?, ?, ?)");
                queryInsert.bind(1, wstos(wscCharname));
                queryInsert.bind(2, bKills ? 1 : 0);
                queryInsert.bind(3, bKills ? 0 : 1);
                queryInsert.bind(4, wstos(wscCharFilename));
                queryInsert.exec();
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }


    void UpdateFFARanking(uint iClientID, bool bKills)
    {
        ConPrint(L"FFARanking Updae\n");

        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        try
        {
            // Open a database file with UTF-8 encoding
            SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

            // Check if the entry already exists in the table
            SQLite::Statement queryExists(db, "SELECT * FROM FFARanking WHERE Charfile = ?");
            queryExists.bind(1, wstos(wscCharFilename));
            bool entryExists = queryExists.executeStep();

            if (entryExists)
            {
                // If the entry exists, update the Kills and Deaths values
                std::string columnName = (bKills ? "Kills" : "Deaths");
                SQLite::Statement queryUpdate(db, "UPDATE FFARanking SET " + columnName + " = " + columnName + " + 1, Charname = ? WHERE Charfile = ?");
                queryUpdate.bind(1, wstos(wscCharname));
                queryUpdate.bind(2, wstos(wscCharFilename));
                queryUpdate.exec();
            }
            else
            {
                // If the entry doesn't exist, insert a new row with the default value of 1 for Kills and Deaths
                SQLite::Statement queryInsert(db, "INSERT INTO FFARanking (Charname, Kills, Deaths, Charfile) VALUES (?, ?, ?, ?)");
                queryInsert.bind(1, wstos(wscCharname));
                queryInsert.bind(2, bKills ? 1 : 0);
                queryInsert.bind(3, bKills ? 0 : 1);
                queryInsert.bind(4, wstos(wscCharFilename));
                queryInsert.exec();
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }

    void CalcRanking(const std::string& tableName)
    {
        ConPrint(L"Task: Calculate " + stows(tableName) + L"...\n");

        try
        {
            // Open a database file
            SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

            // Check if the ranking table already exists and drop it if it does
            std::string calculatedTableName = tableName + "Calculated";
            if (db.tableExists(calculatedTableName))
                db.exec("DROP TABLE " + calculatedTableName);

            // Execute a query to calculate the ranking
            SQLite::Statement query(db, "SELECT Charname, Kills, Deaths, Charfile, (Kills * Kills) / Deaths AS Rating FROM " + tableName + " ORDER BY Rating DESC LIMIT 100");

            // Create a new table for the calculated ranking
            db.exec("CREATE TABLE " + calculatedTableName + " (Charname TEXT, Kills INTEGER, Deaths INTEGER, Charfile TEXT, Rating INTEGER)");

            // Insert the top 100 entries into the calculated ranking table
            while (query.executeStep())
            {
                std::string charname = query.getColumn(0).getString();
                int kills = query.getColumn(1).getInt();
                int deaths = query.getColumn(2).getInt();
                std::string charfile = query.getColumn(3).getString();
                int rating = static_cast<int>(query.getColumn(4).getDouble()); // Umwandlung in int

                SQLite::Statement insertQuery(db, "INSERT INTO " + calculatedTableName + " (Charname, Kills, Deaths, Charfile, Rating) VALUES (?, ?, ?, ?, ?)");
                insertQuery.bind(1, charname);
                insertQuery.bind(2, kills);
                insertQuery.bind(3, deaths);
                insertQuery.bind(4, charfile);
                insertQuery.bind(5, rating);
                insertQuery.exec();
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }



    void WriteFightInfoToCFG(uint iFightID, const std::string& scCharFilename)
    {
        //ConPrint (L"WriteFightInfoToCFG\n");

        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scFightInfo = std::string(szCurDir) + Globals::PVP_FIGHTINFO;

        // Schreibe den Charakterdateinamen und die zugehörige Fight-ID in die CFG-Datei
        IniWrite(scFightInfo, "FightInfo", scCharFilename, std::to_string(iFightID));
    }

    bool IsCharInFightInfo(const std::string& scCharFilename)
    {
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scFightInfo = std::string(szCurDir) + Globals::PVP_FIGHTINFO;

        // Überprüfe, ob der Charakterdateiname als Eintrag in der FightInfo-Sektion existiert
        return IniGetS(scFightInfo, "FightInfo", scCharFilename, "") != "";
    }

    void ClearFightInfo()
    {
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scFightInfo = std::string(szCurDir) + Globals::PVP_FIGHTINFO;

        // Lösche den Inhalt der FightInfo.cfg-Datei
        std::ofstream ofs(scFightInfo, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    void RemoveCharFromFightInfo(const std::string& scCharFilename)
    {
        //ConPrint(L"RemoveFightInfoToCFG " + stows(scCharFilename) + L"\n");


        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scFightInfo = std::string(szCurDir) + Globals::PVP_FIGHTINFO;

        // Lösche den Eintrag des Charakters aus der FightInfo.cfg-Datei
        Tools::FLSRIniDelete(scFightInfo, "FightInfo", scCharFilename);
    }

    void CmdClearPVP(uint iClientID, const std::wstring& wscParam)
    {
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);

        bool isLastPlayer = false;

        for (auto it = ServerFightData.begin(); it != ServerFightData.end(); ++it)
        {
            auto& fight = *it;

            for (auto fightIt = fight.lMembers.begin(); fightIt != fight.lMembers.end(); ++fightIt)
            {
                if (fightIt->wscCharname == wscCharname)
                {
                    fight.lMembers.erase(fightIt);
                    RemoveCharFromFightInfo(wstos(wscCharFilename));

                    // Rückmeldung an den Spieler
                    PrintUserCmdText(iClientID, L"You have been removed from the PvP fight.");

                    // Überprüfen, ob noch mehr als ein Spieler im Fight verbleibt
                    if (fight.lMembers.size() <= 1)
                    {
                        // Der letzte verbleibende Spieler hat den Fight gewonnen
                        if (!fight.lMembers.empty())
                        {
                            std::wstring wscWinnerCharname = fight.lMembers.front().wscCharname;
                            PrintUserCmdText(iClientID, L"The fight has ended. The winner is: " + wscWinnerCharname);
                            PrintUserCmdText(HkGetClientIdFromCharname(wscWinnerCharname), L"The fight has ended. The winner is: " + wscWinnerCharname);
                            RemoveCharFromFightInfo(wstos(fight.lMembers.front().wscCharFilename));
                        }

                        // Markiere den Fight zum Löschen
                        isLastPlayer = true;
                    }

                    // Benachrichtigung an andere Spieler im Fight
                    for (auto& member : fight.lMembers)
                    {
                        if (member.wscCharname != wscCharname)
                        {
                            PrintUserCmdText(HkGetClientIdFromCharname(member.wscCharname), L"Player " + wscCharname + L" has left the PvP fight.");
                        }
                    }

                    break; // Der Spieler wurde gefunden und entfernt, beende die Schleife
                }
            }

            // Wenn der Fight aufgelöst werden soll, entferne ihn aus der Liste
            if (isLastPlayer)
            {
                ServerFightData.erase(it);
                break;
            }
        }

        // Wenn der Spieler nicht in einem PvP-Fight war
        if (!isLastPlayer)
        {
            PrintUserCmdText(iClientID, L"You are not currently participating in any PvP fight.");
        }
    }

    void AddPlayerToFFAFight(uint iFightID, const std::wstring& wscCharname, const std::wstring& wscCharFilename)
    {
        // Suche nach dem entsprechenden FFA-Fight anhand der Fight-ID
        for (auto& fight : ServerFightData)
        {
            if (fight.iFightID == iFightID)
            {
                Member newMember;
                newMember.wscCharname = wscCharname;
                newMember.wscCharFilename = wscCharFilename;
                newMember.iKills = 0;
                newMember.bIsInFight = false;

                fight.lMembers.push_back(newMember);

                return;
            }
        }
    }

    void InvitePlayerToFFAFight(uint iClientID, uint iTargetClientID)
    {
        // Überprüfe, ob der Zielspieler bereits in einem Fight ist
        uint iFightIDTarget = IsInFight(iTargetClientID);
        if (iFightIDTarget != 0)
        {
            PrintUserCmdText(iClientID, L"The target player is already in a fight. You cannot invite them.");
            return;
        }

        // Überprüfe, ob der einladende Spieler in einem Fight ist
        uint iFightID = IsInFight(iClientID);
        if (iFightID == 0)
        {
            PrintUserCmdText(iClientID, L"You are not currently in a fight.");
            return;
        }

        std::wstring wscCharFilenameTarget;
        HkGetCharFileName(ARG_CLIENTID(iTargetClientID), wscCharFilenameTarget);

        // Lade den Zielspieler zum eigenen FFA-Fight ein
        AddPlayerToFFAFight(iFightID, (const wchar_t*)Players.GetActiveCharacterName(iTargetClientID), wscCharFilenameTarget);

        // Sende eine Einladungsnachricht an den Zielspieler
        PrintUserCmdText(iTargetClientID, L"You have been invited to join a FFA fight. Type /pvpaccept to accept the invitation.");

        // Sende eine Bestätigungsnachricht an den einladenden Spieler
        PrintUserCmdText(iClientID, L"You have invited the target player to join the FFA fight.");
    }

    void RemovePlayerFromFight(uint iFightID, const std::wstring& wscCharname) {
        for (auto& fight : ServerFightData) {
            if (fight.iFightID == iFightID) {
                for (auto iter = fight.lMembers.begin(); iter != fight.lMembers.end(); ++iter) {
                    if (iter->wscCharname == wscCharname) {
                        fight.lMembers.erase(iter);
                        break;
                    }
                }
                break;
            }
        }
    }

    void CmdStats(uint iClientID, const std::wstring& wscParam)
    {

        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::string scCharname = wstos(wscCharname);

        // Wandele den Parameter in Kleinbuchstaben um
        std::wstring wscLowerParam = ToLower(wscParam);

        // Prüfe den Parameter und wähle die entsprechende Tabelle aus
        std::string scNormalTable;
        std::string scCalculatedTable;

        if (wscLowerParam == L"pvp")
        {
            CalcRanking("PVPRanking");
            scNormalTable = "PVPRanking";
            scCalculatedTable = "PVPRankingCalculated";
        }
        else if (wscLowerParam == L"pve")
        {
            CalcRanking("PVERanking");
            scNormalTable = "PVERanking";
            scCalculatedTable = "PVERankingCalculated";
        }
        else if (wscLowerParam == L"duel")
        {
            CalcRanking("DuelRanking");
            scNormalTable = "DuelRanking";
            scCalculatedTable = "DuelRankingCalculated";
        }
        else if (wscLowerParam == L"ffa")
        {
            CalcRanking("FFARanking");
            scNormalTable = "FFARanking";
            scCalculatedTable = "FFARankingCalculated";
        }
        else
        {
            PrintUserCmdText(iClientID, L"Invalid parameter. Valid options: pvp, pve, duel, ffa");
            PrintUserCmdText(iClientID, L"Usage: /stats <type>");
            return;
        }

        try
        {
            // Öffne die Datenbankverbindung
            SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

            // Abfrage für die Top 5 Einträge
            std::string scTop5Query = "SELECT Charname, Kills, Deaths, Rating, ROUND(CAST(Kills AS REAL) / NULLIF(Deaths, 0), 2) AS KD, ROW_NUMBER() OVER(ORDER BY Rating DESC) AS Rank FROM " + scCalculatedTable + " ORDER BY Rank ASC LIMIT 5";

            // Abfrage für den Spieler selbst
            std::string scPlayerQuery = "SELECT r.Charname, r.Kills, r.Deaths, c.Rating, ROUND(CAST(r.Kills AS REAL) / NULLIF(r.Deaths, 0), 2) AS KD, (SELECT COUNT(*) FROM " + scCalculatedTable + " WHERE Rating > c.Rating) + 1 AS Rank FROM " + scNormalTable + " r LEFT JOIN " + scCalculatedTable + " c ON r.Charname = c.Charname WHERE r.Charname = '" + scCharname + "'";

            // Abfrage für die Top 5 Einträge
            SQLite::Statement queryTop5(db, scTop5Query);
            PrintUserCmdText(iClientID, L"TOP 5 " + Tools::ToUpper(wscParam) + L"-STATS");
            while (queryTop5.executeStep())
            {
                std::string charname = queryTop5.getColumn(0).getText();
                int kills = queryTop5.getColumn(1).getInt();
                int deaths = queryTop5.getColumn(2).getInt();
                int rating = queryTop5.getColumn(3).getInt();
                double kd = queryTop5.getColumn(4).getDouble();
                int rank = queryTop5.getColumn(5).getInt();

                std::wstring wsckd = std::to_wstring(kd);
                if (wsckd.length() >= 4) {
                    wsckd = wsckd.substr(0, wsckd.length() - 4);
                }

                std::wstring wscEntry = L"[" + std::to_wstring(rank) + L"] " + stows(charname) + L" - Kills: " + std::to_wstring(kills) + L", Deaths: " + std::to_wstring(deaths) + L", Rating: " + std::to_wstring(rating) + L", K/D: " + (wsckd);
                PrintUserCmdText(iClientID, wscEntry);
            }

            // Abfrage für den Spieler selbst
            SQLite::Statement queryPlayer(db, scPlayerQuery);
            PrintUserCmdText(iClientID, L"\nYOUR STATS");
            if (queryPlayer.executeStep())
            {
                std::string charname = queryPlayer.getColumn(0).getText();
                int kills = queryPlayer.getColumn(1).getInt();
                int deaths = queryPlayer.getColumn(2).getInt();
                int rating = queryPlayer.getColumn(3).getInt();
                double kd = queryPlayer.getColumn(4).getDouble();
                int rank = queryPlayer.getColumn(5).getInt();

                std::wstring wsckd = std::to_wstring(kd);
                if (wsckd.length() >= 4) {
                    wsckd = wsckd.substr(0, wsckd.length() - 4);
                }

                std::wstring wscEntry = L"[" + std::to_wstring(rank) + L"] " + stows(charname) + L" - Kills: " + std::to_wstring(kills) + L", Deaths: " + std::to_wstring(deaths) + L", Rating: " + std::to_wstring(rating) + L", K/D: " + (wsckd);
                PrintUserCmdText(iClientID, wscEntry);
            }
            else
            {
                PrintUserCmdText(iClientID, L"No stats found for your character.");
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
            PrintUserCmdText(iClientID, L"Error retrieving statistics. Please try again later.");
        }
    }


}