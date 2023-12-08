#include "Main.h"

namespace Docking {

    mstime msRequestTimeout = 10000;
    float fDockRange = 200.0f;
    bool Carrier_Module = false;
    std::list<CarrierList> lCarrierList;
    std::list<CarrierDockedPlayers> lCarrierDockedPlayers;
    std::list<CarrierDockRequest> lCarrierDockRequest;
    std::list<CarrierConfig> lCarrierConfig;
    std::list<UndockRelocate> lUndockRelocate;

    //Thanks Disco
    bool FLSR_SystemSwitchOutComplete(unsigned int iShip, unsigned int iClientID,unsigned int iSystem,bool bstalkmode) {
        static PBYTE SwitchOut = 0;
        if (!SwitchOut) {
            SwitchOut = (PBYTE)hModServer + 0xf600;

            DWORD dummy;
            VirtualProtect(SwitchOut + 0xd7, 200, PAGE_EXECUTE_READWRITE, &dummy);
        }

        Vector pos;
        Matrix ornt;
        pub::SpaceObj::GetLocation(iShip, pos, ornt);

        // Patch the system switch out routine to put the ship in a
        // system of our choosing.

        SwitchOut[0x0d7] = 0xeb; // ignore exit object
        SwitchOut[0x0d8] = 0x40;
        SwitchOut[0x119] = 0xbb; // set the destination system
        *(PDWORD)(SwitchOut + 0x11a) = iSystem;
        SwitchOut[0x266] = 0x45;               // don't generate warning
        *(float *)(SwitchOut + 0x2b0) = pos.z; // set entry location
        *(float *)(SwitchOut + 0x2b8) = pos.y;
        *(float *)(SwitchOut + 0x2c0) = pos.x;
        *(float *)(SwitchOut + 0x2c8) = ornt.data[2][2];
        *(float *)(SwitchOut + 0x2d0) = ornt.data[1][1];
        *(float *)(SwitchOut + 0x2d8) = ornt.data[0][0];
        *(float *)(SwitchOut + 0x2e0) = ornt.data[2][1];
        *(float *)(SwitchOut + 0x2e8) = ornt.data[2][0];
        *(float *)(SwitchOut + 0x2f0) = ornt.data[1][2];
        *(float *)(SwitchOut + 0x2f8) = ornt.data[1][0];
        *(float *)(SwitchOut + 0x300) = ornt.data[0][2];
        *(float *)(SwitchOut + 0x308) = ornt.data[0][1];
        *(PDWORD)(SwitchOut + 0x388) = 0x03ebc031; // ignore entry object

        //Adminstalk
        if (bstalkmode) {
            //    pub::SpaceObj::SetInvincible(iShip, true, true, 0);
        } else {
            //    pub::SpaceObj::SetInvincible(iShip, false, false, 0);
        }

        Server.SystemSwitchOutComplete(iShip, iClientID);
        SwitchOut[0x0d7] = 0x0f;
        SwitchOut[0x0d8] = 0x84;
        SwitchOut[0x119] = 0x87;
        *(PDWORD)(SwitchOut + 0x11a) = 0x1b8;
        *(PDWORD)(SwitchOut + 0x25d) = 0x1cf7f;
        SwitchOut[0x266] = 0x1a;
        *(float *)(SwitchOut + 0x2b0) = *(float *)(SwitchOut + 0x2b8) = *(float *)(SwitchOut + 0x2c0) = 0;
        *(float *)(SwitchOut + 0x2c8) = *(float *)(SwitchOut + 0x2d0) = *(float *)(SwitchOut + 0x2d8) = 1;
        *(float *)(SwitchOut + 0x2e0) = *(float *)(SwitchOut + 0x2e8) =
        *(float *)(SwitchOut + 0x2f0) = *(float *)(SwitchOut + 0x2f8) =
        *(float *)(SwitchOut + 0x300) = *(float *)(SwitchOut + 0x308) = 0;
        *(PDWORD)(SwitchOut + 0x388) = 0xcf8b178b;

        return true;
    }



    // DockOnProxyCarrierBase Beam
    void DockOnProxyCarrierBase(std::string scBasename, uint iClientID, std::string scCarrierBase, uint iCarrierID) {
        // Konfigpfad
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + Globals::DOCK_CONFIG_FILE;
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        std::string Charname = wstos(wscCharname);
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);
        //std::string base64_Charname = Tools::base64_encode((const unsigned char *)Charname.c_str(), Charname.length());
        std::wstring wscCharnameCarrier = (wchar_t *)Players.GetActiveCharacterName(iCarrierID);
        std::string CharnameCarrier = wstos(wscCharnameCarrier);
        //std::string base64_CharnameCarrier = Tools::base64_encode((const unsigned char *)CharnameCarrier.c_str(), CharnameCarrier.length());
        std::wstring wscCarrierFilename;
        HkGetCharFileName(ARG_CLIENTID(iCarrierID), wscCarrierFilename);
        std::string scCarrierFilename = wstos(wscCarrierFilename);
        

        // Speichere Dockdata (prevent Servercrash dataloss)
        IniWrite(scPluginCfgFile, "Carrier-Docked_" + scFilename, "DockedProxyBase", scBasename);
        IniWrite(scPluginCfgFile, "Carrier-Docked_" + scFilename, "CarrierDockedBase", scCarrierBase);
        IniWrite(scPluginCfgFile, "Carrier-Docked_" + scFilename, "CarrierCharname", scCarrierFilename);

        uint iBaseID;
        if (pub::GetBaseID(iBaseID, scBasename.c_str()) == -4) {
            return;
        }

        uint iSysID;
        pub::Player::GetSystem(iClientID, iSysID);
        Universe::IBase *base = Universe::get_base(iBaseID);

        pub::Player::ForceLand(iClientID, iBaseID); // beam

        // if not in the same system, emulate F1 charload
        if (base->iSystemID != iSysID) {
            Server.BaseEnter(iBaseID, iClientID);
            Server.BaseExit(iBaseID, iClientID);
            std::wstring wscCharFileName;
            HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);
            wscCharFileName += L".fl";
            CHARACTER_ID cID;
            strcpy(cID.szCharFilename, wstos(wscCharFileName.substr(0, 14)).c_str());
            Server.CharacterSelect(cID, iClientID);
        }
        return;
    }

    // UndockProxyCarrierBase
    void UndockProxyBase(uint iCarrierId, uint iClientID, float fx_Undock, float fy_Undock, float fz_Undock, bool bstalkmode) {
        // Konfigpfad
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + Globals::DOCK_CONFIG_FILE;
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        std::string Charname = wstos(wscCharname);
        //std::string base64_Charname = Tools::base64_encode((const unsigned char *)Charname.c_str(), Charname.length());
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);
        std::string IniNamePlayer = "Carrier-Docked_" + scFilename;
        std::wstring wscCharnameCarrier = (wchar_t *)Players.GetActiveCharacterName(iCarrierId);

        // Speichere IDS
        IniWrite(scPluginCfgFile, IniNamePlayer, "ClientID", std::to_string(iClientID));

        // Hole Carrier Schiff
        uint iShip_Carrier;
        pub::Player::GetShip(iCarrierId, iShip_Carrier);

        // Hole Carrier System
        uint iSysIDCarrier;
        pub::Player::GetSystem(iCarrierId, iSysIDCarrier);
        IniWrite(scPluginCfgFile, "Carrier-Docked_" + scFilename, "CarrierSys", std::to_string(iSysIDCarrier));

        // HolePlayer Info (Carrier)
        HKPLAYERINFO p;
        if (HkGetPlayerInfo((wchar_t *)Players.GetActiveCharacterName(iCarrierId), p, false) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR");
            return;
        }

        // Hole Carrier Pos/Rot
        Vector pos;
        Matrix rot;
        pub::SpaceObj::GetLocation(p.iShip, pos, rot);

        // Speichere Carrier Pos - DEBUG
        // IniWrite(scPluginCfgFile, IniNamePlayer, "CarrierX",
        // std::to_string(pos.x)); IniWrite(scPluginCfgFile, IniNamePlayer,
        // "CarrierY", std::to_string(pos.y)); IniWrite(scPluginCfgFile,
        // IniNamePlayer, "CarrierZ", std::to_string(pos.z));

        // Speichere Carrier Rot - DEBUG
        // IniWrite(scPluginCfgFile, IniNamePlayer, "CarrierR1",
        // std::to_string(rot.data[0][0])); IniWrite(scPluginCfgFile, IniNamePlayer,
        // "CarrierR2", std::to_string(rot.data[0][1])); IniWrite(scPluginCfgFile,
        // IniNamePlayer, "CarrierR3", std::to_string(rot.data[0][2]));
        // IniWrite(scPluginCfgFile, IniNamePlayer, "CarrierR4",
        // std::to_string(rot.data[1][0])); IniWrite(scPluginCfgFile, IniNamePlayer,
        // "CarrierR5", std::to_string(rot.data[1][1])); IniWrite(scPluginCfgFile,
        // IniNamePlayer, "CarrierR6", std::to_string(rot.data[1][2]));
        // IniWrite(scPluginCfgFile, IniNamePlayer, "CarrierR7",
        // std::to_string(rot.data[2][0])); IniWrite(scPluginCfgFile, IniNamePlayer,
        // "CarrierR8", std::to_string(rot.data[2][1])); IniWrite(scPluginCfgFile,
        // IniNamePlayer, "CarrierR9", std::to_string(rot.data[2][2]));

        // Hole Player Schiff
        uint iShip_Player;
        pub::Player::GetShip(iClientID, iShip_Player);

        // Hole Player System
        uint iSysIDPlayer;
        pub::Player::GetSystem(iClientID, iSysIDPlayer);

        //Überprüfe ob Player im selben System ist wie der Carrier (Sollte niemals vorkommen bei einem Carrier!)
        if (iSysIDPlayer != iSysIDCarrier) {
            // Setze Schiff auf Unsichtbar um SPOTANTE SELBST EXPLOSIONEN zu
            // vermeiden
            // pub::SpaceObj::SetInvincible(iShip_Player, true, true, 0);

            // Teleport
           // FLSR_SystemSwitchOutComplete(iShip_Player, iClientID, iSysIDCarrier, bstalkmode);
            UndockRelocate NewRelocate;
			NewRelocate.fx_Undock = fx_Undock;
			NewRelocate.fy_Undock = fy_Undock;
			NewRelocate.fz_Undock = fz_Undock;
			NewRelocate.iClientID = iClientID;
			NewRelocate.pos = pos;
			NewRelocate.rot = rot;
			NewRelocate.bStalkMode = bstalkmode;
            NewRelocate.iShip = iShip_Player;
			NewRelocate.iSystem = iSysIDCarrier;
            

            lUndockRelocate.push_back(NewRelocate);



            //PrintUserCmdText(iClientID, L"SYSBEAM");
        }

      

        
        return;
    }

    void DockRequest3000ms() {



        
        // Überprüfe auf DockRequest
        std::list<CarrierDockRequest>::iterator iterDockRequest = lCarrierDockRequest.begin();
        while (iterDockRequest != lCarrierDockRequest.end()) {
            if (!iterDockRequest->bSend) {
                std::wstring Charname = (wchar_t *)Players.GetActiveCharacterName( iterDockRequest->iPlayerID);
                PrintUserCmdText(iterDockRequest->iCarrierID, Charname + L" request to dock. Type /dockaccept " + Charname + L" to accept his request.");

                // Update the Request
                CarrierDockRequest UpdateDockRequest;
                UpdateDockRequest.iPlayerID = iterDockRequest->iPlayerID;
                UpdateDockRequest.iCarrierID = iterDockRequest->iCarrierID;
                UpdateDockRequest.tmRequestTime = iterDockRequest->tmRequestTime;
                UpdateDockRequest.bSend = true;
                UpdateDockRequest.sInterior = iterDockRequest->sInterior;
				UpdateDockRequest.fx_Undock = iterDockRequest->fx_Undock;
				UpdateDockRequest.fy_Undock = iterDockRequest->fy_Undock;
				UpdateDockRequest.fz_Undock = iterDockRequest->fz_Undock;
                lCarrierDockRequest.erase(iterDockRequest);
                lCarrierDockRequest.push_back(UpdateDockRequest);
                iterDockRequest = lCarrierDockRequest.begin();
            }

            iterDockRequest++;
        }
    }

    void HandleUndocking(uint iClientID)
    {
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + Globals::DOCK_CONFIG_FILE;
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        std::string Charname = wstos(wscCharname);
        //std::string base64_Charname = Tools::base64_encode((const unsigned char *)Charname.c_str(), Charname.length());
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);

        //Überprüfe ob der Spieler an einer Carrier-Proxy Base gedockt ist
        std::string DockedProxyBase = IniGetS(scPluginCfgFile, "Carrier-Docked_" + scFilename, "DockedProxyBase", "");
        std::string CarrierDockedBase = IniGetS(scPluginCfgFile, "Carrier-Docked_" + scFilename, "CarrierDockedBase", "");
        if (DockedProxyBase != "" && CarrierDockedBase != "") {
            // Finde den Carrier
            bool CarrierFound = false;
            std::list<CarrierDockedPlayers>::iterator iterDockedPlayers = lCarrierDockedPlayers.begin();
            while (iterDockedPlayers != lCarrierDockedPlayers.end()) {

                // Ist in der Carrierliste
                if (iterDockedPlayers->iPlayerID == iClientID) {
                    // Überprüfe ob Spieler im Space ist
                    uint iShipCarrier;
                    pub::Player::GetShip(iterDockedPlayers->iCarrierID, iShipCarrier);
                    
                    if (iShipCarrier) {
                        // Undock
						UndockProxyBase(iterDockedPlayers->iCarrierID, iClientID, iterDockedPlayers->fx_Undock, iterDockedPlayers->fy_Undock, iterDockedPlayers->fz_Undock, false);
                        CarrierFound = true;
                        return;
                    }
                }

                // Hochzählen
                iterDockedPlayers++;
            }

            // Wenn es den Carrier nicht mehr gibt -> Docke Docke an der CarrierBase
            if (!CarrierFound) {
                PrintUserCmdText(
                    iClientID,
                    L"Carrier not in Space. Docked on last Carrierbase!");
                HkBeam(wscCharname, stows(CarrierDockedBase));
                IniDelSection(scPluginCfgFile, "Carrier-Docked_" + scFilename);
                return;
            }
        }

        // Überprüfe ob der Spieler ein Carrier ist
        std::list<CarrierList>::iterator iterCarrier = lCarrierList.begin();
        while (iterCarrier != lCarrierList.end()) {

            // Spieler ist selbst ein Carrier
            if (iterCarrier->iCarrierID == iClientID) {

                //Überprüfe ob Spielerschiff ein Carrierschiff ist
                std::list<CarrierConfig>::iterator iterCarrierConfig = lCarrierConfig.begin();
                bool CarrierShip = false;
                bool iCarrierSlots = 0;
				float x_Undock = 0.0f;
				float y_Undock = 0.0f;
				float z_Undock = 0.0f;
                
                std::string sCarrierInterior = "";
                // Get ShipArchID
                uint iShipArchIDPlayer;
                pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
                while (iterCarrierConfig != lCarrierConfig.end()) {
                    if (iShipArchIDPlayer == iterCarrierConfig->iShipArch) {

                        CarrierShip = true;
                        iCarrierSlots = iterCarrierConfig->iSlots;
                        sCarrierInterior = iterCarrierConfig->sInterior;
						x_Undock = iterCarrierConfig->fx_Undock;
						y_Undock = iterCarrierConfig->fy_Undock;
						z_Undock = iterCarrierConfig->fz_Undock;
                        
                      
                    }

                    // Hochzählen
                    iterCarrierConfig++;
                }

                if (!CarrierShip) {

                    // Disable Carrier
                    std::list<CarrierList>::iterator iterCarrierDisable = lCarrierList.begin();
                    while (iterCarrierDisable != lCarrierList.end()) {

                        if (iterCarrierDisable->iCarrierID == iClientID) {
                            lCarrierList.erase(iterCarrierDisable);
                           // PrintUserCmdText(iClientID, L"Your ship has no carrier ability!");
                            return;
                        }

                        // Hochzählen
                        iterCarrierDisable++;
                    }
                } else {


                    // Set Carrier Slots & Interior & UndockOffsets
                    std::list<CarrierList>::iterator iterCarrierSlots = lCarrierList.begin();
                    while (iterCarrierSlots != lCarrierList.end()) {

                        if (iterCarrierSlots->iCarrierID == iClientID) {

                            //PrintUserCmdText(iClientID, L"U are now a Carrier!");

                            // Carrier mit Slots & Interior in die Liste
                            CarrierList NewCarrier;
                            NewCarrier.iCarrierID = iterCarrierSlots->iCarrierID;
                            NewCarrier.iBaseDocked = iterCarrierSlots->iBaseDocked;
                            NewCarrier.iDockedPlayers = iterCarrierSlots->iDockedPlayers;
                            NewCarrier.iSlots = iCarrierSlots;
                            NewCarrier.Interior = sCarrierInterior;
							NewCarrier.fx_Undock = x_Undock;
							NewCarrier.fy_Undock = y_Undock;
							NewCarrier.fz_Undock = z_Undock;

                            lCarrierList.erase(iterCarrierSlots);
                            lCarrierList.push_back(NewCarrier);

                            return;
                        }

                        // Hochzählen
                        iterCarrierSlots++;
                    }
                }

                return;
            }

            // Hochzählen
            iterCarrier++;
        }
    }


    void DOCKACCEPT_ALL(uint iClientID) {
        if (Modules::GetModuleState("CarrierModule")) {
			uint iClientCarrierID = iClientID;

           // ConPrint(L"asdf\n");
            // Suche dockrequest
            std::list<Docking::CarrierDockRequest>::iterator iterDockRequest = Docking::lCarrierDockRequest.begin();
            while (iterDockRequest != Docking::lCarrierDockRequest.end()) {

              //  ConPrint(L"asdf1\n");

                
                // Nur eigene dockrequests
                if (iterDockRequest->iCarrierID == iClientCarrierID) {
                 //   ConPrint(L"asdf2\n");


                   // PrintUserCmdText(iClientID, stows(iterDockRequest->sInterior));

                    //Überprüfe die Distanz
                    uint CarrierShip;
                    pub::Player::GetShip(iClientCarrierID, CarrierShip);
                    uint RequestedShip;
                    pub::Player::GetShip(iterDockRequest->iPlayerID, RequestedShip);
                    if (HkDistance3DByShip(CarrierShip, RequestedShip) < Docking::fDockRange) {

                        // Docke Player
                        Docking::CarrierDockedPlayers NewDockedPlayer;
                        NewDockedPlayer.iCarrierID = iClientCarrierID;
                        NewDockedPlayer.iPlayerID = iterDockRequest->iPlayerID;
                        NewDockedPlayer.fx_Undock = iterDockRequest->fx_Undock;
                        NewDockedPlayer.fy_Undock = iterDockRequest->fy_Undock;
                        NewDockedPlayer.fz_Undock = iterDockRequest->fz_Undock;

                        Docking::lCarrierDockedPlayers.push_back(NewDockedPlayer);

                        PrintUserCmdText(iClientCarrierID, L"OK!");
                       // PrintUserCmdText(iClientID, stows(iterDockRequest->sInterior));

                        // Lande Player auf ProxyBase
                        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);

                        std::wstring wscDir;
                        if (!HKHKSUCCESS(HkGetAccountDirName(wscCharname, wscDir)))
                            break;

                        std::wstring wscFile;
                        HkGetCharFileName(wscCharname, wscFile);
                        std::string scCharFile = scAcctPath + wstos(wscDir) + "\\" + wstos(wscFile) + ".fl";
                        std::string scBasename = IniGetS(scCharFile, "Player", "base", "");
                        if (scBasename == "")
                        {
                            scBasename = IniGetS(scCharFile, "Player", "last_base", "");
                        }
                        
                        Docking::DockOnProxyCarrierBase(iterDockRequest->sInterior, iterDockRequest->iPlayerID, scBasename, iClientCarrierID);

                        Docking::lCarrierDockRequest.erase(iterDockRequest);

                    }
                    else 
                    {
                        std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iterDockRequest->iPlayerID);
                        PrintUserCmdText(iClientCarrierID, L"Player " + wscCharname + L" is too far away!");
                        PrintUserCmdText(iterDockRequest->iPlayerID, L"U are too far away to Dock!");
                        break;
                    }
                }
            
            // Hochzählen
            iterDockRequest++;
            }

        }
    }

    
    void ClearCarrier(uint iClientID)
    {
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + Globals::DOCK_CONFIG_FILE;
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::string Charname = wstos(wscCharname);
        //std::string base64_Charname = Tools::base64_encode((const unsigned char *)Charname.c_str(), Charname.length());
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);

        //Überprüfe ob der Spieler an einer Carrier-Proxy Base gedockt ist
        std::string DockedProxyBase = IniGetS(scPluginCfgFile, "Carrier-Docked_" + scFilename, "DockedProxyBase", "");
        std::string CarrierDockedBase = IniGetS(scPluginCfgFile, "Carrier-Docked_" + scFilename, "CarrierDockedBase", "");
        if (DockedProxyBase != "" && CarrierDockedBase != "") {

            uint iDockedProxyBaseID;
            if (pub::GetBaseID(iDockedProxyBaseID, DockedProxyBase.c_str()) == -4) {
                return;
            }
			
            if (iDockedProxyBaseID != ClientInfo[iClientID].iLastExitedBaseID)
            {
                //Undock from Carrier
                IniDelSection(scPluginCfgFile, "Carrier-Docked_" + scFilename);

            }
            

        }
        
    }

}