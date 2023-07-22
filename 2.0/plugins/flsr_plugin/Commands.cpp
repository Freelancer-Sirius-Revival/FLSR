#include "Main.h"

namespace Commands {
    //Test Commands
    /*
    void UserCmd_TESTDEPOT(uint iClientID, const std::wstring& wscParam) {
        Depot::PlayerDepotOpen(iClientID);
        
    }

    void UserCmd_TESTAC(uint iClientID, const std::wstring &wscParam) {
        std::wstring wscType = wscParam;
        std::string scType = wstos(wscType);
        AntiCheat::Reporting::ReportCheater(iClientID, scType, "");
    }

    void UserCmd_TESTCC(uint iClientID, const std::wstring& wscParam) {
        std::wstring wscMsg = wscParam;
        ClientController::Send_ControlMsg(false, iClientID, wscMsg);
    }



    void UserCmd_TESTINSURANCE(uint iClientID, const std::wstring& wscParam) {
        Tools::GetHardpointsFromCollGroup(iClientID);
    }

    void UserCmd_TESTWP(uint iClientID, const std::wstring& wscParam) {
        std::wstring wscMsg = wscParam;
        std::list <CustomMissions::PlayerWaypoint> lWP;
        uint iSysID;
        pub::Player::GetSystem(iClientID, iSysID);
        CustomMissions::PlayerWaypoint TestWP;
		TestWP.iSystemID = iSysID;
        TestWP.iSolarObjectID = 0;
        TestWP.X = "-33356";
        TestWP.Y = "0";
        TestWP.Z = "-500";
        CustomMissions::PlayerWaypoint TestWP2;
        TestWP2.iSystemID = iSysID;
		TestWP2.iSolarObjectID = 0;
        TestWP2.X = "-12345";
        TestWP2.Y = "0";
        TestWP2.Z = "-500";
        CustomMissions::PlayerWaypoint TestWP3;
        TestWP3.iSystemID = iSysID;
		TestWP3.iSolarObjectID = 0;
        TestWP3.X = "-23333";
        TestWP3.Y = "0";
        TestWP3.Z = "-500";
        lWP.push_back(TestWP);
        lWP.push_back(TestWP2);
        lWP.push_back(TestWP3);
        CustomMissions::Send_WPs(iClientID, lWP, true);
    }
    
      void UserCmd_testcloak(uint iClientID, const std::wstring& wscParam) {
        ClientController::Send_ControlMsg(true, iClientID, L"_cloaktoggle");

    }
        */

    // Discord
    void UserCMD_BANK(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("DiscordBot"))
        {
			std::wstring wscCharFileName;
			HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);
			std::string scCharfile = wstos(wscCharFileName);
			std::string scDiscordID = Discord::GetDiscordIDForChar(scCharfile);
            if (scDiscordID == "") // No Link Request
            {
				PrintUserCmdText(iClientID, L"Char not linked!");
				return;
			}
            else
            {

                //Show Balance without Parameter
                if (wscParam == L"")
                {
                    //Get Current Balance
                    std::string scCredits = Discord::GetCreditsForDiscordAccount(scDiscordID);
                    PrintUserCmdText(iClientID, L"Current Balance: " + stows(scCredits));
                    PrintUserCmdText(iClientID, L"Use /bank <amount> to deposit credits");
                }
                else
                {
                    std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);


                    // Parameter übergeben - Betrag auf Discord-Account überweisen

                    // Ist der Char alt genug (Abuse-Prevention - 1h)?
                    int secs = 0;
                    HkGetOnlineTime(wscCharname, secs);
                    if (secs < 3600) {
                        PrintUserCmdText(iClientID, L"ERR insufficient time online");
                        return;
                    }

                    // Betrag aus dem Parameter extrahieren und in einen int umwandeln
                    int amount = std::stoi(wstos(wscParam));

                    // Betrag validieren (muss positiv sein)
                    if (amount <= 0)
                    {
                        PrintUserCmdText(iClientID, L"Invalid amount specified.");
                        return;
                    }

                    // Ist genügend Geld auf dem Charakter?
                    int iCash;
                    HkGetCash(wscCharname, iCash);
                    if (iCash < amount)
                    {
                        //Nicht genügeng Geld
                        PrintUserCmdText(iClientID, L"You don't have enough credits.");
                        return;
                    }

                    
                    // The last error.
                    HK_ERROR err;

                    if ((err = HkAddCash(wscCharname, -amount)) != HKE_OK) {

                        PrintUserCmdText(iClientID, L"Error while updateing ingame balance");
                        return;
                    }

                    // Betrag auf den Discord-Account überweisen
                    if (!Discord::UpdateCreditsForDiscordAccount(scDiscordID, std::to_string(amount), true))
                    {
						PrintUserCmdText(iClientID, L"Error while updating discord balance");
						return;
					}

                    // Erfolgmeldung anzeigen
                    PrintUserCmdText(iClientID, L"Successfully deposited " + wscParam + L" credits to your account.");
                }




			}
		}   


    }

    void UserCMD_LINK(uint iClientID, const std::wstring& wscParam) {

        if (Modules::GetModuleState("DiscordBot"))
        {

            std::wstring wscCharFileName;
            HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);
            std::string scCharfile = wstos(wscCharFileName);
            std::string sha1PW = Tools::sha1(wstos(wscParam));
 

            //Get 2FA
            std::string scValid = Discord::GetValidationForChar(scCharfile);
            if (scValid == "") // No Link Request
            {
                PrintUserCmdText(iClientID, L"No link request!");
                return;
            }
            else if (scValid == "TRUE") // Already Linked
            {
				PrintUserCmdText(iClientID, L"Char already linked!");
				return;
			}


            if (scValid == sha1PW)
            {
                Discord::UpdateValidationForChar(scCharfile);
                PrintUserCmdText(iClientID, L"Char Linked!");
                std::string sscCharname = wstos((wchar_t*)Players.GetActiveCharacterName(iClientID));

                std::string scDiscordID = Discord::GetDiscordIDForChar(scCharfile);
                std::string scMsg = "Char " + sscCharname + " linked!";
                dpp::message dm(scMsg);
                Discord::DMMessage NewMessage;
                NewMessage.DiscordUserID = scDiscordID;
                NewMessage.DiscordMessage = dm;

                {
                    // Mutex sperren
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    Discord::lDMMessages.push_back(NewMessage);

                    Discord::CharManager_UpdateCharname(scCharfile, sscCharname);
                } // Mutex wird hier automatisch freigegeben
            }
            else
            {
				PrintUserCmdText(iClientID, L"Wrong Pass!");
			}

        }
    }

    ///// PVP


    void UserCmd_pvpduel(uint iClientID, const std::wstring& wscParam) {

        if (Modules::GetModuleState("PVP"))
        {
            PVP::CmdFight(iClientID, wscParam, PVP::PVPTYPE_DUEL);
        }
    }

    void UserCmd_pvpffa(uint iClientID, const std::wstring& wscParam) {

        if (Modules::GetModuleState("PVP"))
        {
            PVP::CmdFight(iClientID, wscParam, PVP::PVPTYPE_FFA);
        }
    }

    void UserCmd_pvpaccept(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("PVP"))
        {
            PVP::CmdAcceptPVP(iClientID, wscParam);
        }

    }
    
    void UserCmd_pvpclear(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("PVP"))
        {
            PVP::CmdClearPVP(iClientID, wscParam);
        }

    }

    void UserCmd_stats(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("PVP"))
        {
            PVP::CmdStats(iClientID, wscParam);
        }

    }

    void UserCmd_pvpinvite(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("PVP"))
        {
            // Überprüfe ob Spieler im Space ist
            uint iShip;
            pub::Player::GetShip(iClientID, iShip);
            if (!iShip)
            {
                PrintUserCmdText(iClientID, L"Please undock!");
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

            PVP::InvitePlayerToFFAFight(iClientID, iTargetClientID);

        }

    }
	

    void UserCmd_PLAYERHUNT(uint iClientID, const std::wstring& wscParam) {

        if (Modules::GetModuleState("PlayerHunt"))
        {
            PlayerHunt::Start_PlayerHunt(iClientID, wscParam);
        }
    }

    void UserCmd_HELP(uint iClientID, const std::wstring& wscParam) {

        
        //Create Popup struct
        PopUp::PopUpBox NewPopUpBox;
        NewPopUpBox.iClientID = iClientID;
        NewPopUpBox.iHead = 520006;
        NewPopUpBox.iBody = 520007;
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

    void UserCmd_Tag(uint iClientID, const std::wstring& wscParam)
    {
        std::wstring wscError[] =
        {
            L"Error: Invalid parameters",
            L"Usage: /tag <faction name>"
        };     

		std::wstring wscParameters = wscParam;
        wscParameters = Trim(wscParameters);

        if (wscParameters.length())
        {
            wscParameters = ToLower(wscParameters);
            uint iGroup = Tools::GetiGroupOfFaction(wscParameters);
      
            
            
            if (iGroup)
            {
                int iRep;
                pub::Player::GetRep(iClientID, iRep);
                uint iIDS = Reputation::get_name(iGroup);
                std::wstring wscFaction = HkGetWStringFromIDS(iIDS);
                float fRep;
                pub::Reputation::GetGroupFeelingsTowards(iRep, iGroup, fRep);
                if (fRep < 0.6f)
                {
                    PrintUserCmdText(iClientID, L"Error: your reputation of %g to " + wscFaction + L" is less than the required %g.", fRep, 0.6f);
                }
                else
                {
                    pub::Reputation::SetAffiliation(iRep, iGroup);
                    PrintUserCmdText(iClientID, L"Affiliation changed to " + wscFaction);
                }
            }
            else
            {
                PrintUserCmdText(iClientID, L"Error: could not find faction");
            }
        }
        else
        {
            PrintUserCmdText(iClientID, L"Error: could not find faction");
        }
    }
    
    void UserCmd_CLOAK(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("CloakModule"))
        {
            
            std::wstring wscCharFileName;
            HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);

            uint iShip = 0;
            pub::Player::GetShip(iClientID, iShip);
            if (!iShip)
            {
                PrintUserCmdText(iClientID, L"Error: You are docked.");
                return;
            }
			
            if (ClientInfo[iClientID].bTradelane)
            {
				//PrintUserCmdText(iClientID, L"Error: You are in a tradelane.");
				return;
            }

            if (Cloak::mPlayerCloakData[wscCharFileName].bCanCloak)
            {
                //PrintUserCmdText(iClientID, L"CLOAK TOGGLED SERVRSIDE");
                Cloak::StartCloakPlayer(iClientID);

            }


        }
    }

    void UserCmd_UNCLOAK(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("CloakModule"))
        {
            Cloak::UncloakPlayer(iClientID);



        }
    }

    /*void UserCMD_INSURANCE_AUTOSAVE(uint iClientID, const std::wstring& wscParam) {
        if (Modules::GetModuleState("InsuranceModule"))
        {
            uint ship;
            pub::Player::GetShip(iClientID, ship);

            //Check On/Off
            CAccount* acc = Players.FindAccountFromClientID(iClientID);
            std::wstring wscAccDir;
            HkGetAccountDirName(acc, wscAccDir);
            std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;
            std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
            std::string Charname = wstos(wscCharname);
            std::wstring wscFilename;
            HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
            std::string scFilename = wstos(wscFilename);
            std::string sAutoInsurance = IniGetS(scUserFile, scFilename, "INSURANCE-State", "no");

            if (sAutoInsurance == "no") {
                if (!ship) {
                    if (Insurance::Insurance_Module) {
                        PrintUserCmdText(iClientID, L"Automatic insurance activated!");
                        IniWrite(scUserFile, scFilename, "INSURANCE-State", "yes");
                        Insurance::CalcInsurance(iClientID, false, false);

                        //New Payed Insurance
                        Insurance::BookInsurance(iClientID, false);
                    }
                }
                else {
                    PrintUserCmdText(iClientID, L"You can only apply for an insurance when docked.");
                }
            } else {
                PrintUserCmdText(iClientID, L"Automatic insurance disabled!");
                IniWrite(scUserFile, scFilename, "INSURANCE-State", "no");
                
                //DeleteInsurance
                char szCurDir[MAX_PATH];
                GetCurrentDirectory(sizeof(szCurDir), szCurDir);
                std::string scInsuranceStore = std::string(szCurDir) + INSURANCE_STORE;
                std::string sInsurancepath = scInsuranceStore + scFilename + ".cfg";
                remove(sInsurancepath.c_str());
                
                //New Free Insurance
                Insurance::BookInsurance(iClientID, true);

            }
        }
    }
    */

    void UserCMD_INSURANCE_AUTOSAVE(uint iClientID, const std::wstring& wscParam) {

        using namespace Insurance;

        // Check Modul
        if (!Modules::GetModuleState("InsuranceModule")) {
            return;
        }

        // Charname
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::string scCharname = wstos(wscCharname);

        // Check InsuranceType - Konfig
        CAccount* acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;

        // Charfilename
        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::string scCharFilename = wstos(wscCharFilename);

        // Lese die INI-Werte
        bool bMines = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Mines", "OFF") == "ON");
        bool bProjectiles = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Projectiles", "OFF") == "ON");
        bool bCountermeasures = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Countermeasures", "OFF") == "ON");
        bool bShieldBatteries = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "ShieldBatteries", "OFF") == "ON");
        bool bNanobots = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Nanobots", "OFF") == "ON");
        bool bEquipment = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Equipment", "OFF") == "ON");

        // Berechne die insuranceMask
        InsuranceType insuranceMask = InsuranceType::None;

        if (bMines)
            insuranceMask |= InsuranceType::Mines;
        if (bProjectiles)
            insuranceMask |= InsuranceType::Projectiles;
        if (bCountermeasures)
            insuranceMask |= InsuranceType::Countermeasures;
        if (bShieldBatteries)
            insuranceMask |= InsuranceType::ShieldBatteries;
        if (bNanobots)
            insuranceMask |= InsuranceType::Nanobots;
        if (bEquipment)
            insuranceMask |= InsuranceType::Equipment;
      
        // Prüfen, ob alle sechs Typen festgelegt sind
        if (bool allTypesSet = (bMines && bProjectiles && bCountermeasures && bShieldBatteries && bNanobots && bEquipment))
            insuranceMask |= InsuranceType::All;

        std::list<Insurance::InsuranceType> insuranceTypes = Insurance::GetInsuranceTypesFromMask(insuranceMask);

        // Check InsuranceTypes
        bool isAllInsured = false;
        bool isNoneInsured = false;
        for (const auto& type : insuranceTypes) {
            std::string typeString = Insurance::GetInsuranceTypeString(type);
            //ConPrint(stows(typeString + "\n"));

            if (type == Insurance::InsuranceType::All) {
                isAllInsured = true;
                break;
            }


            if (type == Insurance::InsuranceType::None && insuranceTypes.size() == 1) {
                isNoneInsured = true;
                break;
            }
        }

        //ConPrint(std::to_wstring(insuranceTypes.size()) + L"\n");

        // Insurance TypeString
        std::string scInsuranceType;

        if (isAllInsured) {
            // PrintUserCmdText(iClientID, L"ALL Types are Insured");
            scInsuranceType = "All";
        }
        else if (isNoneInsured) {
            // PrintUserCmdText(iClientID, L"None");
            scInsuranceType = "None";
        }
        else {
            bool firstType = true;  // Variable zur Verfolgung des ersten Versicherungstyps

            for (const auto& type : insuranceTypes) {
                if (type == Insurance::InsuranceType::None)
                    continue;

                std::string typeString = Insurance::GetInsuranceTypeString(type);

                if (!firstType) {
                    scInsuranceType += ", ";  // Komma nur hinzufügen, wenn bereits ein Eintrag vorhanden ist
                }
                else {
                    firstType = false;  // Setze firstType auf false, nachdem der erste Versicherungstyp hinzugefügt wurde
                }

                scInsuranceType += typeString;
                // PrintUserCmdText(iClientID, L"Insurance Type: " + stows(typeString));
            }
        }

        //Insurance Store
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scInsuranceStore = std::string(szCurDir) + INSURANCE_STORE;

        //Get Insurance State
        bool bisInsured = Insurance::insurace_exists(scInsuranceStore + scCharFilename + ".cfg");

        //Get Insurance Data
        std::list<Insurance::RestoreEquip> lInsuredEquip;
        int Store_iWorth;
        int Store_iCountEquip;
        bool bFreeInsurance;

        if (bisInsured)
        {
            //Get Insurance Data
            Store_iWorth = IniGetI(scInsuranceStore + scCharFilename + ".cfg", "INSURANCE", "Worth", 0);
            Store_iCountEquip = IniGetI(scInsuranceStore + scCharFilename + ".cfg", "INSURANCE", "EquipCount", 0);
            bFreeInsurance = IniGetB(scInsuranceStore + scCharFilename + ".cfg", "INSURANCE", "FreeInsurance", false);

            //Read Insurance from Store
            int Equiploop = 0;
            while (Equiploop <= Store_iCountEquip) {

                //Read Equip
                bool bMission = IniGetB(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "bMission", false);
                bool bMounted = IniGetB(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "bMounted", true);
                float fStatus = IniGetF(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "fStatus", 1);
                std::string hardpoint = IniGetS(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "hardpoint", "");
                uint iArchID = IniGetI(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "iArchID", 0);
                uint iCount = IniGetI(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "iCount", 0);
                uint iID = IniGetI(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "iID", 0);

                //Prepare Data
                CacheString newhardpoint;
                const char* cshardpoint = hardpoint.c_str();
                newhardpoint.value = StringAlloc(cshardpoint, false);

                //Create Equip
                CARGO_INFO NewCargo_Info;
                NewCargo_Info.bMission = bMission;
                NewCargo_Info.bMounted = bMounted;
                NewCargo_Info.fStatus = fStatus;
                NewCargo_Info.hardpoint = newhardpoint;
                NewCargo_Info.iArchID = iArchID;
                NewCargo_Info.iCount = iCount;
                NewCargo_Info.iID = iID;

                Insurance::RestoreEquip NewEquip;
                NewEquip.CARGO_INFO = NewCargo_Info;
                NewEquip.bItemisFree = IniGetB(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "bItemisFree", true);
                NewEquip.fPrice = IniGetF(scInsuranceStore + scCharFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "fPrice", 0);
                lInsuredEquip.push_back(NewEquip);

                Equiploop++;
            }
		}

        // Checke Parameter
        const std::string& scMsg = wstos(GetParamToEnd(wscParam, ' ', 0));
        if (scMsg == "") {
            // Kein Parameter angegeben, zeige aktuellen Versicherungstyp an
            PrintUserCmdText(iClientID, L"Current insurance type: " + stows(scInsuranceType));

            // Zusammenfassung des aktuellen Versicherungsstatus
            std::wstring wscInsuranceStatus = L"Your insurance is currently ";
            wscInsuranceStatus += (bisInsured) ? L"ACTIVE" : L"INACTIVE";
            PrintUserCmdText(iClientID, wscInsuranceStatus);

            if (bisInsured && scInsuranceType != "None")
			{
                // Anzahl und Wert der versicherten Ausrüstung
                PrintUserCmdText(iClientID,L"Total insured worth: " + std::to_wstring(Store_iWorth) + L" Credits");
            }
            // Berechnung falls Gedockt
            //Check if player is docked
            uint iShip = 0;
            pub::Player::GetShip(iClientID, iShip);
            if (!iShip)
            {
                PrintUserCmdText(iClientID, L"The projected value of the new insurance is: " + std::to_wstring(CalcInsurance(iClientID)) + L" Credits");
            }

            
            return;
        }
        else {

            //Check if player is docked
            uint iShip = 0;
            pub::Player::GetShip(iClientID, iShip);
            if (iShip)
            {
                PrintUserCmdText(iClientID, L"ERR: You can only apply for an insurance when docked.");
                return;
            }

            // Aufteilen der Eingabezeichenkette nach Leerzeichen
            std::istringstream iss(scMsg);
            std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };

            for (const auto& token : tokens) {
                Insurance::InsuranceType insuranceType = Insurance::GetInsuranceTypeFromString(token);

                //INI-Datei aktualisieren
                switch (insuranceType) {
                case Insurance::InsuranceType::All:
                    bMines = bProjectiles = bCountermeasures = bShieldBatteries = bNanobots = bEquipment = true;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Mines", "ON");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Projectiles", "ON");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Countermeasures", "ON");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "ShieldBatteries", "ON");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Nanobots", "ON");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Equipment", "ON");
                    // Enable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "ON");

                    //New Payed Insurance
                    Insurance::BookInsurance(iClientID, false);

                    PrintUserCmdText(iClientID, L"Settings changed: All types activated");
                    break;
                case Insurance::InsuranceType::Mines:
                    bMines = !bMines;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Mines", bMines ? "ON" : "OFF");
                    // Enable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "ON");

                    //New Payed Insurance
                    Insurance::BookInsurance(iClientID, false);

                    PrintUserCmdText(iClientID, (bMines ? L"Settings changed: Mines activated" : L"Settings changed: Mines deactivated"));
                    break;
                case Insurance::InsuranceType::Projectiles:
                    bProjectiles = !bProjectiles;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Projectiles", bProjectiles ? "ON" : "OFF");
                    // Enable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "ON");

                    //New Payed Insurance
                    Insurance::BookInsurance(iClientID, false);

                    PrintUserCmdText(iClientID, (bProjectiles ? L"Settings changed: Projectiles activated" : L"Settings changed: Projectiles deactivated"));
                    break;
                case Insurance::InsuranceType::Countermeasures:
                    bCountermeasures = !bCountermeasures;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Countermeasures", bCountermeasures ? "ON" : "OFF");
                    // Enable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "ON");

                    //New Payed Insurance
                    Insurance::BookInsurance(iClientID, false);

                    PrintUserCmdText(iClientID, (bCountermeasures ? L"Settings changed: Countermeasures activated" : L"Settings changed: Countermeasures deactivated"));
                    break;
                case Insurance::InsuranceType::ShieldBatteries:
                    bShieldBatteries = !bShieldBatteries;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "ShieldBatteries", bShieldBatteries ? "ON" : "OFF");
                    // Enable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "ON");

                    //New Payed Insurance
                    Insurance::BookInsurance(iClientID, false);

                    PrintUserCmdText(iClientID, (bShieldBatteries ? L"Settings changed: ShieldBatteries activated" : L"Settings changed: ShieldBatteries deactivated"));
                    break;
                case Insurance::InsuranceType::Nanobots:
                    bNanobots = !bNanobots;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Nanobots", bNanobots ? "ON" : "OFF");
                    // Enable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "ON");

                    //New Payed Insurance
                    Insurance::BookInsurance(iClientID, false);

                    PrintUserCmdText(iClientID, (bNanobots ? L"Settings changed: Nanobots activated" : L"Settings changed: Nanobots deactivated"));
                    break;
                case Insurance::InsuranceType::Equipment:
                    bEquipment = !bEquipment;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Equipment", bEquipment ? "ON" : "OFF");
                    // Enable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "ON");

                    //New Payed Insurance
                    Insurance::BookInsurance(iClientID, false);

                    PrintUserCmdText(iClientID, (bEquipment ? L"Settings changed: Equipment activated" : L"Settings changed: Equipment deactivated"));
                    break;
                case Insurance::InsuranceType::None:
                    bMines = bProjectiles = bCountermeasures = bShieldBatteries = bNanobots = bEquipment = false;
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Mines", "OFF");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Projectiles", "OFF");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Countermeasures", "OFF");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "ShieldBatteries", "OFF");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Nanobots", "OFF");
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "Equipment", "OFF");
                    // Disable Auto Insurance
                    IniWrite(scUserFile, "INSURANCE-" + scCharFilename, "AutoInsurance", "OFF");

                    //New Free Insurance
                    Insurance::BookInsurance(iClientID, true);

                    PrintUserCmdText(iClientID, L"Settings changed: All types deactivated");
                    break;
                case Insurance::InsuranceType::Invalid:
                    PrintUserCmdText(iClientID, L"Settings changed: Unknown type!");
                    return;
                default:
                    break;
                }                    
            }
        }

    }


    void UserCmd_UV(uint iClientID, const std::wstring &wscParam) {
        std::wstring Chat = wscParam;
        std::wstring Charname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        Chat::HkSendUChat(Charname, Chat);

        //Discord
        Discord::ChatMessage ChatMsg;
        ChatMsg.wscCharname = Charname;
        ChatMsg.wscChatMessage = Chat;


        {
            // Mutex sperren
            std::lock_guard<std::mutex> lock(m_Mutex);

            // Chat-Nachricht zur Liste hinzufügen
            Discord::lChatMessages.push_back(ChatMsg);
        } // Mutex wird hier automatisch freigegeben


    }

    void UserCmd_MODREQUEST(uint iClientID, const std::wstring &wscParam) {
        std::wstring Chat = wscParam;
        std::wstring Charname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
        PrintUserCmdText (iClientID, L"Your request has been sent to the moderators.");
        //Discord
        Discord::ChatMessage ChatMsg;
        ChatMsg.wscCharname = Charname;
        ChatMsg.wscChatMessage = Chat;


        {
            // Mutex sperren
            std::lock_guard<std::mutex> lock(m_Mutex);

            // Chat-Nachricht zur Liste hinzufügen
            Discord::lModMessages.push_back(ChatMsg);
        } // Mutex wird hier automatisch freigegeben
    }

    void UserCMD_ENABLECARRIER(uint iClientID, const std::wstring &wscParam) {
        if (Modules::GetModuleState("CarrierModule")) {
            bool bLastBaseUsed = false;
            
            // GetBase
            uint iBase = 0;
            pub::Player::GetBase(iClientID, iBase);

            
            
            // Abbrechen falls nicht gedockt
            if (iBase == 0) {
                // Get the current character name
                std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
                std::wstring wscBasename;
                if (HkFLIniGet(wscCharname, L"base", wscBasename) != HKE_OK)
                    return;

                pub::GetBaseID(iBase, wstos(wscBasename).c_str());
                
                bLastBaseUsed = true;
			
            }
            
            //Überprüfe ob Spieler ein Carrier sein kann (Schiff)
            // Get ShipArchID
            uint iShipArchIDPlayer;
            pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
            std::list<Docking::CarrierConfig>::iterator iterCarrierConfig = Docking::lCarrierConfig.begin();
            bool bCarrierShip = false;
            int iconfigslots;
            while (iterCarrierConfig != Docking::lCarrierConfig.end()) 
            {
                if (iShipArchIDPlayer == iterCarrierConfig->iShipArch) {
					if (iterCarrierConfig->iSlots >= 0) {
						bCarrierShip = true;
						iconfigslots = iterCarrierConfig->iSlots;
                        break;
					}


                }

                iterCarrierConfig++;
            }
            if (!bCarrierShip)
                return;


            
            // Überprüfe ob der Spieler bereits ein Carrier ist
            bool isCarrier = false;
            std::list<Docking::CarrierList>::iterator iterCarrier =
                Docking::lCarrierList.begin();
            while (iterCarrier != Docking::lCarrierList.end()) {

                if (iterCarrier->iCarrierID == iClientID) {
                    isCarrier = true;
                    break;
                }

                // Hochzählen
                iterCarrier++;
            }

            if (isCarrier) {
               // PrintUserCmdText(iClientID, L"U are already a Carrier!");
                return;
            }

            // Carrier in die Liste
            Docking::CarrierList NewCarrier;
            NewCarrier.iCarrierID = iClientID;
            NewCarrier.iBaseDocked = iBase;
            NewCarrier.iDockedPlayers = 0;
            NewCarrier.iSlots = iconfigslots;
            Docking::lCarrierList.push_back(NewCarrier);

            // CarrierEnabled
            
            if (bLastBaseUsed)
            {
                Docking::HandleUndocking(iClientID);
            }
        }
    }

    void UserCMD_DOCKREQUEST(uint iClientID, const std::wstring &wscParam) {
        if (Modules::GetModuleState("CarrierModule")) {

            // Überprüfe ob Spieler im Space ist
            uint iShip;
            pub::Player::GetShip(iClientID, iShip);
            if (!iShip) {
                PrintUserCmdText(iClientID, L"Please undock!");
                return;
            }

            // Überprüfe auf Target
            uint iTargetShip;
            pub::SpaceObj::GetTarget(iShip, iTargetShip);
            if (!iTargetShip) {
                PrintUserCmdText(iClientID, L"Please select a target!");
                return;
            }

            // Überprüfe ob Target ein Spieler ist
            uint iTargetClientID = HkGetClientIDByShip(iTargetShip);
            if (!iTargetClientID) {
                PrintUserCmdText(iClientID, L"Please select a player!");
                return;
            }
            
            //Überprüfe ob Spieler ein Carrier ist
			bool bisCarrier = false;
            std::list<Docking::CarrierList>::iterator iterCarrier1 = Docking::lCarrierList.begin();
            while (iterCarrier1 != Docking::lCarrierList.end()) {

				//
                (L"CarrierID: %u \n", iterCarrier1->iCarrierID);
                
                // Spieler ist selbst ein Carrier
                if (iterCarrier1->iCarrierID == iClientID) {
                    return;

                }
                iterCarrier1++;
            }

            // Überprüfe ob der Spieler ein Carrier ist
            std::list<Docking::CarrierList>::iterator iterCarrier = Docking::lCarrierList.begin();
            while (iterCarrier != Docking::lCarrierList.end()) {

                // Spieler ist selbst ein Carrier
                if (iterCarrier->iCarrierID == iClientID) {
                    PrintUserCmdText(iClientID, L"You are a carrier yourself!");
                    return;
                }

                // TargetSpieler ist ein Carrier
                if (iterCarrier->iCarrierID == iTargetClientID) {


                   // ConPrint(std::to_wstring(iterCarrier->iSlots) + L"\n");
                    //ConPrint(std::to_wstring(iterCarrier->iCarrierID) + L"\n");
                    
                    // Slots
                    if (iterCarrier->iSlots == 0) {
                        //PrintUserCmdText(iClientID, L"Carrier has no free capacity.");
                        return;
                    }
                    if (iterCarrier->iDockedPlayers >= iterCarrier->iSlots) {
                        //PrintUserCmdText(iClientID, L"Docked: " + std::to_wstring(iterCarrier->iDockedPlayers));
                        //PrintUserCmdText(iClientID, L"Slots" + std::to_wstring(iterCarrier->iSlots));

                        PrintUserCmdText(iClientID, L"Carrier has no free capacity.");
                        return;
                    }

                    //Gibt es schon einen Request
                    std::list<Docking::CarrierDockRequest>::iterator iterRequests = Docking::lCarrierDockRequest.begin();
                    while (iterRequests != Docking::lCarrierDockRequest.end()) {
						if (iterRequests->iPlayerID == iClientID) {
							PrintUserCmdText(iClientID, L"You already have a request!");
							return;
						}
                        iterRequests++;
                    }
                    

                    // Distance
                    if (HkDistance3DByShip(iShip, iTargetShip) < Docking::fDockRange) {
                        // Carrier in die Liste
                        Docking::CarrierDockRequest NewDockRequest;
                        NewDockRequest.iPlayerID = iClientID;
                        NewDockRequest.iCarrierID = iterCarrier->iCarrierID;
                        NewDockRequest.tmRequestTime = timeInMS();
                        NewDockRequest.iBaseCarrierDocked = iterCarrier->iBaseDocked;
                        NewDockRequest.sInterior = iterCarrier->Interior;
                        NewDockRequest.bSend = false;
						NewDockRequest.fx_Undock = iterCarrier->fx_Undock;
                        NewDockRequest.fy_Undock = iterCarrier->fy_Undock;
                        NewDockRequest.fz_Undock = iterCarrier->fz_Undock;
                        
                        Docking::lCarrierDockRequest.push_back(NewDockRequest);
                        PrintUserCmdText(iClientID, L"Dock request send!");
                    } else {
                        PrintUserCmdText(iClientID, L"Too far too dock!");
                        return;
                    }
                }

                // Hochzählen
                iterCarrier++;
            }
        }
    }

    void UserCMD_DOCKACCEPT(uint iClientID, const std::wstring &wscParam) {
        if (Modules::GetModuleState("CarrierModule")) {

            std::wstring Charname = wscParam;
            uint requestedClientId = HkGetClientIdFromCharname(Charname);
            // Nur wenn die ClientID gültig ist den Spieler Docken
            if (HkIsValidClientID(requestedClientId)) {

                // Suche dockrequest
                std::list<Docking::CarrierDockRequest>::iterator iterDockRequest = Docking::lCarrierDockRequest.begin();
                while (iterDockRequest != Docking::lCarrierDockRequest.end()) {
                    // Nur eigene dockrequests
                    if (iterDockRequest->iCarrierID == iClientID) {

                        if (iterDockRequest->iPlayerID == requestedClientId) {

                            //PrintUserCmdText(iClientID, stows(iterDockRequest->sInterior));

                            //Überprüfe die Distanz
                            uint CarrierShip;
                            pub::Player::GetShip(iClientID, CarrierShip);
                            uint RequestedShip;
                            pub::Player::GetShip(iterDockRequest->iPlayerID, RequestedShip);
                            if (HkDistance3DByShip(CarrierShip, RequestedShip) < Docking::fDockRange) {

                                // Docke Player
                                Docking::CarrierDockedPlayers NewDockedPlayer;
                                NewDockedPlayer.iCarrierID = iClientID;
                                NewDockedPlayer.iPlayerID = requestedClientId;
								NewDockedPlayer.fx_Undock = iterDockRequest->fx_Undock;
								NewDockedPlayer.fy_Undock = iterDockRequest->fy_Undock;
								NewDockedPlayer.fz_Undock = iterDockRequest->fz_Undock;
                                
                                Docking::lCarrierDockedPlayers.push_back( NewDockedPlayer);

                                PrintUserCmdText(iClientID, L"OK!");
                                //PrintUserCmdText(iClientID, stows(iterDockRequest->sInterior));

                                // Lande Player auf ProxyBase
                                std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

                                std::wstring wscDir;
                                if (!HKHKSUCCESS(HkGetAccountDirName(wscCharname, wscDir)))
                                    return;

                                std::wstring wscFile;
                                HkGetCharFileName(wscCharname, wscFile);
                                std::string scCharFile = scAcctPath + wstos(wscDir) + "\\" + wstos(wscFile) + ".fl";
                                std::string scBasename = IniGetS(scCharFile, "Player", "base", "");
                                if (scBasename == "")
                                {
                                    scBasename = IniGetS(scCharFile, "Player", "last_base", "");
                                }
                                
                                Docking::DockOnProxyCarrierBase(iterDockRequest->sInterior, requestedClientId, scBasename, iClientID);

                                Docking::lCarrierDockRequest.erase(iterDockRequest);

                            } else {
                                PrintUserCmdText(iClientID, L"Player is too far away!");
                                PrintUserCmdText(requestedClientId, L"U are too far away to Dock!");
                                return;
                            }
                        }
                    }
                    // Hochzählen
                    iterDockRequest++;
                }

            } else {
                PrintUserCmdText(iClientID, L"No such Charname found!");
                return;
            }
        }
    }

    /** Process a give cash command */
    void UserCMD_SendCash(uint iClientID, const std::wstring &wscParam) {
        // The last error.
        HK_ERROR err;

        // Get the current character name
        std::wstring wscCharname =
            (const wchar_t *)Players.GetActiveCharacterName(iClientID);

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

    void UserCMD_SendCash$(uint iClientID, const std::wstring &wscParam) {
        // The last error.
        HK_ERROR err;

        // Get the current character name
        std::wstring wscCharname =
            (const wchar_t *)Players.GetActiveCharacterName(iClientID);

        // Get the parameters from the user command.

        std::wstring wscClientID = GetParam(wscParam, L' ', 0);
        uint iClientIDTarget = ToInt(wscClientID);
        if (!HkIsValidClientID(iClientIDTarget) ||
            HkIsInCharSelectMenu(iClientIDTarget)) {
            PrintUserCmdText(iClientID, L"Error: Invalid client-id");
            return;
        }

        std::wstring wscTargetCharname =
            (const wchar_t *)Players.GetActiveCharacterName(iClientIDTarget);
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
    void UserCMD_Contributor(uint iClientID, const std::wstring &wscParam) {
        ClientController::Send_ControlMsg(true, iClientID, L"_INFOCARDUPDATE Contributor");

        FmtStr caption(0, 0);
        caption.begin_mad_lib(PopUp::iContributor_Head);
        caption.end_mad_lib();

        FmtStr message(0, 0);
        message.begin_mad_lib(PopUp::iContributor_Body);
        message.end_mad_lib();

        pub::Player::PopUpDialog(iClientID, caption, message, POPUPDIALOG_BUTTONS_CENTER_OK);
        return;
    }

    //Not used commands

    /*
    
        // MissionGroup Bug
    void UserCMD_Clear(uint iClientID, const std::wstring &wscParam) {

        Tools::HkClearMissionBug(iClientID);
        return;
    }

    */
    
    //ADMIN COMMANDS
    
    // ADMIN Stalk
    void AdminCmd_Stalk(CCmds *cmds, std::wstring Charname) {

        // Rechte Check
        if (!(cmds->rights & RIGHT_SUPERADMIN)) {
            cmds->Print(L"ERR No permission\n");
            return;
        }

        // Hole ClientID
        uint iClientID = HkGetClientIdFromCharname(cmds->GetAdminName());

        PrintUserCmdText(iClientID, Charname);

        // Teleport
        if (Charname == L"") {
			Docking::UndockProxyBase(iClientID, iClientID, 0.0f, 0.0f, 0.0f, true);
        } else {
            uint iCarrierID = HkGetClientIdFromCharname(Charname);
			Docking::UndockProxyBase(iCarrierID, iClientID, 0.0f, 0.0f, 0.0f, true);
        }

        cmds->Print(L"OK\n");
    }
	
	//ChangeModuleState
    void AdminCmd_SwitchModuleState(CCmds* cmds, std::wstring wscModulename) {
		
		// Rechte Check
		if (!(cmds->rights & RIGHT_SUPERADMIN)) {
			cmds->Print(L"ERR No permission\n");
			return;
		}
		
		//Switch Module State
        Modules::SwitchModuleState(wstos(wscModulename));
        cmds->Print(L"OK\n");
    }

    USERCMD UserCmds[] = {
        {L"/uv", UserCmd_UV},
        {L"/modrequest", UserCmd_MODREQUEST},
        {L"/enablecarrier", UserCMD_ENABLECARRIER},
        {L"/dockrequest", UserCMD_DOCKREQUEST},
        {L"/dockaccept", UserCMD_DOCKACCEPT},
        {L"/sendcash", UserCMD_SendCash},
        {L"/sendcash$", UserCMD_SendCash$},
        {L"/contributor", UserCMD_Contributor},
        {L"/autoinsurance", UserCMD_INSURANCE_AUTOSAVE},
        {L"/cloak", UserCmd_CLOAK},
		{L"/uncloak", UserCmd_UNCLOAK},
        {L"/help", UserCmd_HELP},
        {L"/tag", UserCmd_Tag},
        {L"/playerhunt", UserCmd_PLAYERHUNT},
        {L"/pvpduel", UserCmd_pvpduel},
        {L"/pvpffa", UserCmd_pvpffa},
        {L"/pvpaccept", UserCmd_pvpaccept},
        {L"/pvpclear", UserCmd_pvpclear},
        {L"/pvpinvite", UserCmd_pvpinvite},
        {L"/stats", UserCmd_stats},
        {L"/link", UserCMD_LINK},
        {L"/bank", UserCMD_BANK},



        //Not Used Commands
        /*
        
         {L"/clear", UserCMD_Clear},

        */

		//Test Commands
        /*{L"/testcloak", UserCmd_testcloak},
        {L"/testac", UserCmd_TESTAC},
        {L"/testcc", UserCmd_TESTCC},
        {L"/testdepot", UserCmd_TESTDEPOT},
        {L"/testins", UserCmd_TESTINSURANCE},
        */

    };

    // User command processing
    bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd) {
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
                returncode = SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command,
                                                         // return immediatly
                return true;
            }
        }
        returncode = DEFAULT_RETURNCODE; // we did not handle the command, so let
                                         // other plugins or FLHook kick in
        return false;
    }

    // Admin Callback
    void CmdHelp_Callback(CCmds *classptr) {
        returncode = DEFAULT_RETURNCODE;
        classptr->Print(L"stalk <charname>\n");
    }

    // Admin command processing
    bool ExecuteCommandString_Callback(CCmds *cmds, const std::wstring &wscCmd) {
        returncode = DEFAULT_RETURNCODE;
        if (IS_CMD("stalk")) {
            returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            AdminCmd_Stalk(cmds, cmds->ArgStr(1));
            return true;
        }
        if (IS_CMD("switchModulestate")) {
            returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            AdminCmd_SwitchModuleState(cmds, cmds->ArgStr(1));
            return true;
        }
        return false;
    }
}