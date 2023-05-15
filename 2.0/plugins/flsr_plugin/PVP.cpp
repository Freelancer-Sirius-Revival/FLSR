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

        for (const auto& fight : ServerFightData) {
            for (const auto& member : fight.lMembers) {
                if (member.bIsInDuel && member.wscCharname == wscCharname) {
                    return fight.iFightID;
                }
            }
        }
        return 0;
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
    
    Member GetPVPMember(uint iClientID)
    {
        // Get the current character name
        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

        for (const auto& fight : ServerFightData) {
            for (const auto& member : fight.lMembers) {
                if (member.bIsInDuel && member.wscCharname == wscCharname) {
                    return member;
                }
            }
        }
		return Member();
    }
    
    void HandleKill(uint iClientKillerID) {
        std::wstring killerCharName = (const wchar_t*)Players.GetActiveCharacterName(iClientKillerID);
        //While Fights
        // Update the fight in the ServerFightData
        for (auto& f : ServerFightData) {
			//While Members
			// Update the member in the ServerFightData
			for (auto& m : f.lMembers) {
				if (m.wscCharname == killerCharName) {
					m.iKills++;
                    
					//Update the Changed Member
					m = f.lMembers.back();

                    
				}
			}

			//Update the Changed Fight if Fight is not a FFA
			if (f.ePVPType != PVPTYPE_FFA) {
			    f.iFightsRemaining = f.iFightsRemaining - 1;
            }
			f = ServerFightData.back();
            
		}        

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

        //Check for DeathType - PVP Kill
        if (DeathType == Tools::PVP)
        {
            // The iClientKillerID must be a valid player
            if (!Tools::isValidPlayer(iClientKillerID, false))
                return;

            // Check if player is in the same fight
            uint iFightIDKiller = IsInFight(iClientKillerID);
            if (iFightIDClient == iFightIDKiller)
            {
                //No Death in a Fight
                return;
            }

            //HandleKill
            HandleKill(iClientKillerID);
        }

    }

}