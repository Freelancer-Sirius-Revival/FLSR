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

   void HandleKill(uint iClientKillerID, PVPType ePVPType) {
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
                   //ConPrint(L"Kills: %u\n", updatedMember.iKills);

                   // Suche manuell nach dem entsprechenden Element und aktualisiere es
                   for (auto it = fight.lMembers.begin(); it != fight.lMembers.end(); ++it) {
                       if (it->wscCharname == member.wscCharname) {
                           *it = updatedMember; // Aktualisiere das Element in der Liste

                           // Verringere die Anzahl der übrigen Fights um eins und aktualisiere ServerFightData
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

       ConPrint(L"TESTDEATH-HandleKill\n");
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
                   ConPrint(L"Kills: %u ", kills);
                   ConPrint(L"Charname: %s\n", member.wscCharname.c_str());                  
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
               }

               // Remove the fight from ServerFightData
               it = ServerFightData.erase(it);
               if (it == ServerFightData.end())
                   break; // Exit the loop if all fights have been processed
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
               if (member.bIsInDuel && member.wscCharname == wscCharnameClient) {
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

    void CmdFight(uint iClientID, const std::wstring& wscParam, PVP::PVPType ePVPType)
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
            PVPType activeFightPVPType = GetActiveFightPVPType(iFightIDTarget);
            if (activeFightPVPType == PVPTYPE_DUEL || activeFightPVPType == PVPTYPE_RANKED)
            {
                PrintUserCmdText(iClientID, L"Target is already in a " + GetWStringFromPVPTYPE(activeFightPVPType) + L" fight!");
                return;
            }
            else if (activeFightPVPType == PVPTYPE_FFA)
            {
                // Füge den Spieler zum vorhandenen FFA-Fight hinzu
                for (auto& fight : ServerFightData)
                {
                    if (fight.iFightID == iFightIDTarget)
                    {
                        Member newMember;
                        newMember.wscCharname = wscCharname;
                        newMember.iKills = 0;
                        newMember.bIsInDuel = true;

                        fight.lMembers.push_back(newMember);

                        // Benachrichtige alle Mitglieder des FFA-Fights
                        std::wstring message = wscCharname + L" has joined the " + GetWStringFromPVPTYPE(activeFightPVPType) + L" fight!";
                        for (const auto& member : fight.lMembers)
                        {
                            if (member.wscCharname != wscCharname)
                            {
                                // Benachrichtigung an andere Mitglieder
                                PrintUserCmdText(HkGetClientIdFromCharname(member.wscCharname), message);
                            }
                            else
                            {
                                // Personalisierte Benachrichtigung an den hinzugefügten Spieler
                                PrintUserCmdText(iClientID, L"You have joined the " + GetWStringFromPVPTYPE(activeFightPVPType) + L" fight!");
                            }
                        }

                        return;
                    }
                }

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
        newFight.lMembers.push_back({ wscCharname, 0, true }); //Herausforderer
        newFight.lMembers.push_back({ wscTargetCharname, 0, false }); //Herausgeforderter

        // Setze den PVP-Typ
        newFight.ePVPType = ePVPType;

		// Setze die Anzahl der Fights
		newFight.iFights = iFights;
		newFight.iFightsRemaining = iFights;

        // Füge den neuen Kampf zur Liste hinzu
        ServerFightData.push_back(newFight);

        // Gib beiden Spielern ein Feedback
        std::wstring wscMessageTarget;
        std::wstring wscMessageClient;

        wscMessageTarget = L"You have been challenged to a " + GetWStringFromPVPTYPE(ePVPType) + L" by " + wscCharname + L"!";
        wscMessageClient = L"You have challenged " + wscTargetCharname + L" to a  " + GetWStringFromPVPTYPE(ePVPType) + L"!";
 
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
                        //ConPrint(L"Member: " + member.wscCharname + L"\n");
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
                HandleKill(iClientKillerID, ePVPType);

                //CheckLastRound
                CheckLastRound();

                //Update Killed Ranking
                UpdateFFARanking(iClientID, false);
            }

        }

    }

    void UpdateDuelRanking(uint iClientID, bool bKills)
    {

        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        ConPrint(L"UpdateDuelRanking\n");
        try
        {
            // Open a database file
            SQLite::Database db(SQL::scDbName, SQLite::OPEN_READWRITE);

            // Check if the entry already exists in the table
            SQLite::Statement queryExists(db, "SELECT * FROM DuelRanking WHERE Charfile = '" + wstos(wscCharFilename) + "'");
            ConPrint(L"Query prepared\n");
            // Print Query
            ConPrint(L"Query: " + stows(queryExists.getQuery().c_str()) + L"\n");
            bool entryExists = queryExists.executeStep();

            if (entryExists)
            {
                ConPrint(L"Entry exists\n");
                // If the entry exists, update the Kills and Deaths values
                std::string columnName = (bKills ? "Kills" : "Deaths");
                SQLite::Statement queryUpdate(db, "UPDATE DuelRanking SET " + columnName + " = " + columnName + " + 1, Charname = '" + wstos(wscCharname) + "' WHERE Charfile = '" + wstos(wscCharFilename) + "'");
                ConPrint(L"Query: " + stows(queryUpdate.getQuery().c_str()) + L"\n");
                queryUpdate.exec();
            }
            else
            {
                ConPrint(L"Entry doesn't exist\n");
                // If the entry doesn't exist, insert a new row with the default value of 1 for Kills and Deaths
                SQLite::Statement queryInsert(db, "INSERT INTO DuelRanking (Charname, Kills, Deaths, Charfile) VALUES ('" + wstos(wscCharname) + "', " + (bKills ? "1, 0" : "0, 1") + ",'" + wstos(wscCharFilename) + "')");                
                ConPrint(L"Query prepared\n");
                ConPrint(L"Query: " + stows(queryInsert.getQuery().c_str()) + L"\n");
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

        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        ConPrint(L"UpdateFFARanking\n");
        try
        {
            // Open a database file
            SQLite::Database db(SQL::scDbName, SQLite::OPEN_READWRITE);

            // Check if the entry already exists in the table
            SQLite::Statement queryExists(db, "SELECT * FROM FFARanking WHERE Charfile = '" + wstos(wscCharFilename) + "'");
            ConPrint(L"Query prepared\n");
            // Print Query
            ConPrint(L"Query: " + stows(queryExists.getQuery().c_str()) + L"\n");
            bool entryExists = queryExists.executeStep();

            if (entryExists)
            {
                ConPrint(L"Entry exists\n");
                // If the entry exists, update the Kills and Deaths values
                std::string columnName = (bKills ? "Kills" : "Deaths");
                SQLite::Statement queryUpdate(db, "UPDATE FFARanking SET " + columnName + " = " + columnName + " + 1, Charname = '" + wstos(wscCharname) + "' WHERE Charfile = '" + wstos(wscCharFilename) + "'");
                ConPrint(L"Query: " + stows(queryUpdate.getQuery().c_str()) + L"\n");
                queryUpdate.exec();
            }
            else
            {
                ConPrint(L"Entry doesn't exist\n");
                // If the entry doesn't exist, insert a new row with the default value of 1 for Kills and Deaths
                SQLite::Statement queryInsert(db, "INSERT INTO FFARanking (Charname, Kills, Deaths, Charfile) VALUES ('" + wstos(wscCharname) + "', " + (bKills ? "1, 0" : "0, 1") + ",'" + wstos(wscCharFilename) + "')");
                ConPrint(L"Query prepared\n");
                ConPrint(L"Query: " + stows(queryInsert.getQuery().c_str()) + L"\n");
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
            SQLite::Database db(SQL::scDbName, SQLite::OPEN_READWRITE);

            // Check if the ranking table already exists and drop it if it does
            std::string calculatedTableName = tableName + "Calculated";
            if (db.tableExists(calculatedTableName))
                db.exec("DROP TABLE " + calculatedTableName);

            // Execute a query to calculate the ranking
            SQLite::Statement query(db, "SELECT Charname, (Kills * Kills) / Deaths AS Rating FROM " + tableName + " ORDER BY Rating DESC LIMIT 100");

            // Create a new table for the calculated ranking
            db.exec("CREATE TABLE " + calculatedTableName + " (Charname TEXT, Rating REAL)");

            // Insert the top 100 entries into the calculated ranking table
            while (query.executeStep())
            {
                std::string charname = query.getColumn(0).getString();
                double rating = query.getColumn(1).getDouble();

                SQLite::Statement insertQuery(db, "INSERT INTO " + calculatedTableName + " (Charname, Rating) VALUES (?, ?)");
                insertQuery.bind(1, charname);
                insertQuery.bind(2, rating);
                insertQuery.exec();
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }




}