#include "Main.h"

namespace Hooks {


    //PopUpDialog
    void __stdcall PopUpDialog(unsigned int iClientID, unsigned int buttonClicked) {
        returncode = DEFAULT_RETURNCODE;

        //ConPrint(L"PopUpDialog %u %u\n", iClientID, buttonClicked);
        PopUp::HandleButtonClick(iClientID, buttonClicked);
    }

    //CharacterSelect
    void __stdcall CharacterSelect(struct CHARACTER_ID const &cId, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;
      

        //InfoCardUpdate
        ClientController::Send_ControlMsg(true, iClientID, L"_INFOCARDUPDATE CharacterSelect");

        //Update BaseState
        ClientController::Send_ControlMsg(false, iClientID, L"_ResetBaseState");       

        //NewPlayerMessage
        Tools::HkNewPlayerMessage(iClientID, cId);
		
        //CloakModule
        if (Modules::GetModuleState("CloakModule"))
        {
            Commands::UserCmd_UNCLOAK(iClientID, L"");
            ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
        }
    }

    // LaunchComplete
    void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip) {
        returncode = DEFAULT_RETURNCODE;

        //Get ClientID
        uint iClientID = HkGetClientIDByShip(iShip);
        if (!iClientID)
            return;

        //AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::DataGrab::CharnameToFLHOOKUSER_FILE(iClientID);
            AntiCheat::SpeedAC::Init(iClientID);
            AntiCheat::TimingAC::Init(iClientID);
            AntiCheat::PowerAC::Init(iClientID);
        }
		
        //Update InfoCards
        ClientController::Send_ControlMsg(true, iClientID, L"_INFOCARDUPDATE LaunchComplete");
        
        //Update BaseState
        ClientController::Send_ControlMsg(false, iClientID, L"_InSpaceState");

        //Show WelcomePopUp
        if (Modules::GetModuleState("WelcomeMSG"))
        {
            PopUp::WelcomeBox(iClientID);
        }
		
        //ProxyUndocking
        if (Modules::GetModuleState("CarrierModule"))
        {
            Docking::ClearCarrier(iClientID);
            Docking::HandleUndocking(iClientID);
        }
		
		//Insurance
        if (Modules::GetModuleState("InsuranceModule"))
        {
            //Insurance-CheckInsurance Booked

            std::pair<bool, bool> Booking = Insurance::CheckInsuranceBooked(iClientID);

            if (Booking.first) {
                Insurance::CreateNewInsurance(iClientID, Booking.second);
            }

            //Insurance-PlayerDied
            Insurance::PlayerDiedEvent(false, iClientID);
        }
		
        if (Modules::GetModuleState("CloakModule"))
        {
            Cloak::InstallCloak(iClientID);
            Commands::UserCmd_UNCLOAK(iClientID, L"");
        }
        
        //Test
        //int iWorth = (int)Tools::CalcDisabledHardpointWorth(iClientID);
        //PrintUserCmdText(iClientID, std::to_wstring(iWorth));
        //Depot::GetEquipname(iClientID);
    }

    // HkCb_AddDmgEntry
    void __stdcall HkCb_AddDmgEntry(DamageList *dmg, unsigned short p1, float damage, enum DamageEntry::SubObjFate fate) {
        returncode = DEFAULT_RETURNCODE;

        //NPCs always do damage
        if (!dmg->is_inflictor_a_player())
        {
            dmg->add_damage_entry(p1, damage, fate);
        }
        
       // ConPrint(std::to_wstring(fate) + L"\n");
         






        // Debug
        // PrintUserCmdText(iDmgTo, L"HIT");
    }

    void SendDeathMsg(const std::wstring& wscMsg, uint iSystemID, uint iClientIDVictim, uint iClientIDKiller) {
        returncode = DEFAULT_RETURNCODE;

        //ConPrint(wscMsg + L"\n");

        std::wstring victim, killer;
		Tools::eDeathTypes DeathType;
        // Extract victim and type from the death message
        if (wscMsg.find(L"was killed by an NPC") != std::wstring::npos) {
                    size_t victimStart = wscMsg.find(L"Death: ") + 7;
                    size_t victimEnd = wscMsg.find(L" was killed by an NPC");
                    victim = wscMsg.substr(victimStart, victimEnd - victimStart);
                    DeathType = Tools::PVE;

        }
        else if (wscMsg.find(L"was killed by an admin") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" was killed by an admin");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);
            DeathType = Tools::ADMIN;
        }
        else if (wscMsg.find(L" was killed by ") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" was killed by ");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);

            size_t killerStart = victimEnd + 15; // length of " was killed by "
            size_t killerEnd = wscMsg.find(L" (", killerStart);
            killer = wscMsg.substr(killerStart, killerEnd - killerStart);

            size_t typeStart = killerEnd + 2; // skip the " ("
            size_t typeEnd = wscMsg.find(L")", typeStart);
            DeathType = Tools::PVP;
        }
        else if (wscMsg.find(L"killed himself") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" killed himself");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);

            size_t typeStart = wscMsg.find_last_of(L"(") + 1;
            size_t typeEnd = wscMsg.find_last_of(L")");
            DeathType = Tools::KILLEDHIMSELF;

        }
        else if (wscMsg.find(L"committed suicide") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" committed suicide");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);
            DeathType = Tools::SUICIDE;
        }
        else if (wscMsg.find(L" has died") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" has died");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);
            DeathType = Tools::HASDIED;
        }


        // Remove whitespaces from victim
        victim.erase(std::remove_if(victim.begin(), victim.end(), [](unsigned char c) { return std::isspace(c); }), victim.end());
    
        // Remove whitespaces from killer
        killer.erase(std::remove_if(killer.begin(), killer.end(), [](unsigned char c) { return std::isspace(c); }), killer.end());

        // Get the victim's and killer's client IDs
        //uint victimClientID = HkGetClientIdFromCharname(victim);
       // uint killerClientID = HkGetClientIdFromCharname(killer);

		// Print the victim's and killer's client IDs for testing
		//ConPrint(L"Victim Client ID: " + std::to_wstring(victimClientID) + L"\n");
		//ConPrint(L"Killer Client ID: " + std::to_wstring(killerClientID) + L"\n");
        
        //PlayerHunt
        if (Modules::GetModuleState("PlayerHunt"))
        {
			PlayerHunt::CheckDied(iClientIDVictim, iClientIDKiller, DeathType);
		}
        
        //PVP
        if (Modules::GetModuleState("PVP"))
        {
            PVP::CheckDied(iClientIDVictim, iClientIDKiller, DeathType);
        }

        
    }



    
    //ShipDestroyed
    void __stdcall ShipDestroyed(DamageList *_dmg, DWORD *ecx, uint iKill) {
        returncode = DEFAULT_RETURNCODE;


      
   
            CShip* cship = (CShip*)ecx[4];
            uint iClientID = cship->GetOwnerPlayer();

            if (iClientID) { // a player was killed

                // Insurance-PlayerDied
                if (iClientID != 0) {
				    if (Modules::GetModuleState("InsuranceModule"))
				    {
					    Insurance::PlayerDiedEvent(true, iClientID);
				    }
                }
			
                //EquipWhiteList
                if (Modules::GetModuleState("EquipWhiteListModule"))
                {
                    uint iShipArchIDPlayer;
                    pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
                    EquipWhiteList::SendList(iShipArchIDPlayer, iClientID, false);
                }

                if (Modules::GetModuleState("CloakModule"))
                {
                    Commands::UserCmd_UNCLOAK(iClientID, L"");
                    ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
                }
            }
        
        
    }

    //BaseEnter_AFTER
    void __stdcall BaseEnter_AFTER(unsigned int iBaseID,unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;
        //Insurance
        if (Modules::GetModuleState("InsuranceModule"))
        {
            //Insurance Use
            if (!Insurance::CheckPlayerDied(iClientID))
            {
                Insurance::UseInsurance(iClientID);
            }
            else {
                Insurance::UseInsurance(iClientID);
            }

            //Insurance ReNew
            Insurance::ReNewInsurance(iClientID);

            
        }
		
		//AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::SpeedAC::Init(iClientID);
            AntiCheat::TimingAC::Init(iClientID);
        }	

        //PlayerHunt
        if (Modules::GetModuleState("PlayerHunt"))
        {
			PlayerHunt::CheckDock(iBaseID, iClientID);
        }

        //EquipWhiteList
        if (Modules::GetModuleState("EquipWhiteListModule"))
        {
            uint iShipArchIDPlayer;
            pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
            EquipWhiteList::SendList(iShipArchIDPlayer, iClientID, false);
        }

		//Cloak
        if (Modules::GetModuleState("CloakModule"))
        {
            Commands::UserCmd_UNCLOAK(iClientID, L"");
            ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
        }

        //PathSelection
        if (Modules::GetModuleState("PathSelection"))
        {
            PathSelection::Install_Unlawful(iClientID);
        }
        
        //Update BaseState
        ClientController::Send_ControlMsg(false, iClientID, L"_OnBaseState");
    }

	//SPObjUpdate
    void __stdcall SPObjUpdate(struct SSPObjUpdateInfo const &ui, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;
        //AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::TimingAC::CheckTimeStamp(ui, iClientID);
            AntiCheat::SpeedAC::CheckSpeedCheat(ui, iClientID);
        }
    }

	//SubmitChat
    void __stdcall SubmitChat(CHAT_ID cId, unsigned long lP1, void const *rdlReader, CHAT_ID cIdTo, int iP2) {
        returncode = DEFAULT_RETURNCODE;

        ClientController::Handle_ClientControlMsg(cId, lP1, rdlReader, cIdTo, iP2);
    }

	//Dock_Call
    int __cdecl Dock_Call(unsigned int const& iShip, unsigned int const& iDockTarget, int iCancel, enum DOCK_HOST_RESPONSE response) {
        returncode = DEFAULT_RETURNCODE;
		
        uint iClientID = HkGetClientIDByShip(iShip);
        if (HkIsValidClientID(iClientID)) {
            //AC
            if (Modules::GetModuleState("ACModule"))
            {
                AntiCheat::SpeedAC::iDunno3(iShip, iDockTarget, iCancel, response);
            }
        
        
            //CloakModule
            if (Modules::GetModuleState("CloakModule"))
            {
                if (!Cloak::Check_Dock_Call(iShip, iDockTarget, iCancel, response))
                {
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
                }
            }

            //PathSelection
            if (Modules::GetModuleState("PathSelection"))
            {
				if (!PathSelection::Check_BlockedGate(iShip))
				{
					returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				}
                
            }
        }

        return 0;
    }
    
    void __stdcall RequestEvent(int iIsFormationRequest, unsigned int iShip, unsigned int iDockTarget, unsigned int p4, unsigned long p5, unsigned int iClientID)
    {
        returncode = DEFAULT_RETURNCODE;
        
        //CloakModule
        if (Modules::GetModuleState("CloakModule"))
        {
            if (!Cloak::Check_RequestEventFormaDocking( iIsFormationRequest, iShip, iDockTarget, p4, p5, iClientID))
            {
                returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            }
        }
    }
	
    // Called when a gun hits something
    void __stdcall SPMunitionCollision(struct SSPMunitionCollisionInfo const& ci, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;

        //AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::SpeedAC::vDunno1(iClientID, 10000);
        }
    }

	//JumpInComplete
    void __stdcall JumpInComplete(unsigned int iSystemID, unsigned int iShipID) {
        returncode = DEFAULT_RETURNCODE;

        uint iClientID = HkGetClientIDByShip(iShipID);
        if (!iClientID)
            return;

        //AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::SpeedAC::Init(iClientID);
            AntiCheat::SpeedAC::vDunno2(iClientID);
            AntiCheat::SpeedAC::vDunno1(iClientID, 10000);
            AntiCheat::TimingAC::Init(iClientID);
            AntiCheat::PowerAC::Init(iClientID);
        }

        //PlayerHunt
        if (Modules::GetModuleState("PlayerHunt"))
        {
			PlayerHunt::CheckSystemReached(iClientID, iSystemID);
        }
    }
	
    void __stdcall SystemSwitchOutComplete(unsigned int iShip, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;
        		
		//AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::SpeedAC::vDunno2(iClientID);
            AntiCheat::SpeedAC::vDunno1(iClientID, 20000);
        }

		//CarrierModul
		if (Modules::GetModuleState("CarrierModule"))
		{
            //ConPrint(L"SystemSwitchOutComplete\n");

            struct PlayerData* pd = 0;
            while (pd = Players.traverse_active(pd)) {
                 
				
             
                // Überprüfe auf DockRequest
                std::list<Docking::UndockRelocate>::iterator iterRelocate = Docking::lUndockRelocate.begin();
                while (iterRelocate != Docking::lUndockRelocate.end()) {

                    if (pd->iOnlineID == iterRelocate->iClientID)
                    {
                        //ConPrint(L"PlayerData: %u\n", iterRelocate->iClientID);
                        
                        Docking::FLSR_SystemSwitchOutComplete(iterRelocate->iShip, iterRelocate->iClientID, iterRelocate->iSystem, iterRelocate->bStalkMode);

                       // PrintUserCmdText(iClientID, L"RELOCATE");
                        
						Vector pos = iterRelocate->pos;
						Matrix rot = iterRelocate->rot;
                        
                        


                        TranslateX(pos, rot, iterRelocate->fy_Undock);
                        TranslateY(pos, rot, iterRelocate->fz_Undock);
                        TranslateZ(pos, rot, iterRelocate->fx_Undock);


                        /*
                        PrintUserCmdText(iterRelocate->iClientID, L"XOffset: " + stows(std::to_string(iterRelocate->fx_Undock)));
                        PrintUserCmdText(iterRelocate->iClientID, L"YOffset: " + stows(std::to_string(iterRelocate->fy_Undock)));
                        PrintUserCmdText(iterRelocate->iClientID, L"ZOffset: " + stows(std::to_string(iterRelocate->fz_Undock)));
                        */
                        
                        HkRelocateClient(iterRelocate->iClientID, pos, rot);


                        pub::SpaceObj::SetInvincible(iterRelocate->iShip, false, false, 0);
                        
                        Docking::lUndockRelocate.erase(iterRelocate);
                    }


                    
                    iterRelocate++;
                }
                



            }
            

		}


        

    }

    /// Clear client info when a client connects.
    void __stdcall ClearClientInfo(unsigned int iClientID) {
		//AC
        if (Modules::GetModuleState("ACModule"))
        {
          AntiCheat::SpeedAC::Init(iClientID);
          AntiCheat::TimingAC::Init(iClientID);
          AntiCheat::PowerAC::Init(iClientID);

        }

        //Cloak
        if (Modules::GetModuleState("CloakModule"))
        {
    
          Cloak::InstallCloak(iClientID);
          Commands::UserCmd_UNCLOAK(iClientID, L"");
          ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");

        }

        ClientController::Send_ControlMsg(true, iClientID, L"_INFOCARDUPDATE ClearClientInfo");
    }

    void __stdcall FireWeapon(unsigned int iClientID, struct XFireWeaponInfo const& wpn) {
        returncode = DEFAULT_RETURNCODE;
		
		//AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::PowerAC::FireWeapon(iClientID, wpn);
        }

        //CloakModule
        if (Modules::GetModuleState("CloakModule"))
        {
            if (Cloak::Check_Cloak(iClientID))
            {
                returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            }
        }
    }

    void __stdcall PlayerLaunch_After(unsigned int iShip, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;

        //AC
        if (Modules::GetModuleState("ACModule"))
        {
            AntiCheat::SpeedAC::Init(iClientID);
            AntiCheat::TimingAC::Init(iClientID);
            AntiCheat::PowerAC::Init(iClientID);
        }
		
		//Cloak
        if (Modules::GetModuleState("CloakModule"))
        {
           //Install CLoak on Spawn
           Cloak::InstallCloak(iClientID);
           Commands::UserCmd_UNCLOAK(iClientID, L"");
           ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
			
        }

        //EquipWhiteList
        if (Modules::GetModuleState("EquipWhiteListModule"))
        {
            uint iShipArchIDPlayer;
            pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
            EquipWhiteList::SendList(iShipArchIDPlayer, iClientID, false);        
        }

		//CarrierModule
        if (Modules::GetModuleState("CarrierModule"))
        {
            Commands::UserCMD_ENABLECARRIER(iClientID, L"");
            //ConPrint(L"Spawn\n");
        }

        //Update BaseState
        ClientController::Send_ControlMsg(false, iClientID, L"_InSpaceState");
    }

    void __stdcall ReqAddItem(unsigned int goodID, char const* hardpoint, int count,float status, bool mounted, uint iClientID) {
        returncode = DEFAULT_RETURNCODE;
        
		//EquipWhiteList
        if (Modules::GetModuleState("EquipWhiteListModule"))
        {
            uint iShipArchIDPlayer;
            pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
            EquipWhiteList::SendList(iShipArchIDPlayer, iClientID, false);
            if (EquipWhiteList::ReqAddItem_CheckEquipWhiteList(goodID, hardpoint, count, status, mounted, iClientID)) {
                //PrintUserCmdText(iClientID, L"Item not added");
                //pub::Audio::PlaySoundEffect(iClientID, CreateID("no_place_to_mount"));
                returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            }
        }
    }

    void __stdcall ReqShipArch_AFTER(unsigned int iArchID, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;
		
        //EquipWhiteList
        if (Modules::GetModuleState("EquipWhiteListModule"))
        {
            EquipWhiteList::SendList(iArchID, iClientID, false);
        }

        //CloakModule
        if (Modules::GetModuleState("CloakModule"))
        {
            Commands::UserCmd_UNCLOAK(iClientID, L"");
            ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
        }
    }

    void __stdcall ReqEquipment(class EquipDescList const& edl, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;
		
        //EquipWhiteList
        if (Modules::GetModuleState("EquipWhiteListModule"))
        {
            uint iShipArchIDPlayer;
            pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
            EquipWhiteList::SendList(iShipArchIDPlayer, iClientID, false);
        }
		
    }

    void __stdcall GoTradelane(unsigned int iClientID, struct XGoTradelane const& gtl) {
        returncode = DEFAULT_RETURNCODE;
		
        //CloakModule
        if (Modules::GetModuleState("CloakModule"))
        {
            if (!Cloak::Check_GoTradelane(iClientID,gtl))
            {
                returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            }
        }

    }
	
    void __stdcall DisConnect(unsigned int iClientID, enum EFLConnection state) {
        returncode = DEFAULT_RETURNCODE;
		
		//CloakModule
        if (Modules::GetModuleState("CloakModule"))
        {
            try {
                //Install CLoak on Spawn
                Cloak::InstallCloak(iClientID);
                Commands::UserCmd_UNCLOAK(iClientID, L"");
                ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
            }
            catch (...) {}
        }
        
        //PlayerHunt
        if (Modules::GetModuleState("PlayerHunt"))
        {
            PlayerHunt::CheckDisConnect(iClientID);
        }
        //PVP
        if (Modules::GetModuleState("PVP"))
        {
            PVP::CheckDisConnect(iClientID, PVP::DisconnectReason::DISCONNECTED);
        }
    }
    
    void __stdcall CreateNewCharacter_After(struct SCreateCharacterInfo const& si, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;
        /*
        std::wstring wscCharname(si.wszCharname);
        ConPrint(L"CreateNewCharacter: %s", wscCharname);
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        ConPrint(L"NewCharfile: %s", wscFilename);
        */
        //SetLastNewChar to Account FlhookIni
    }


}