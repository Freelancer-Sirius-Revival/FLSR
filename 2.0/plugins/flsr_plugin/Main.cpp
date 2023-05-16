// includes
#include "Main.h"

PLUGIN_RETURNCODE returncode;
_CRCAntiCheat CRCAntiCheat_FLSR;

/// Lade Konfig
void LoadSettings() {
    returncode = DEFAULT_RETURNCODE;
    // Konfigpfad
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    std::string scPluginCfgFile = std::string(szCurDir) + PLUGIN_CONFIG_FILE;

    //Load HashMap
    if (Tools::ReadIniNicknames())
    {
        ConPrint(L"\n----------LOADING FLSR-HashMap----------\n");
        ConPrint(L"Loaded HashMap with %d entries\n", Tools::mNicknameHashMap.size());
        ConPrint(L"\n");
    }
    
    //Load FuseMap
   /* if (FuseControl::ReadIniFuseConfig())
    {
        ConPrint(L"\n----------LOADING FLSR-FuseMap----------\n");
        ConPrint(L"Loaded FuseMap with %d entries\n", FuseControl::mFuseMap.size());
        ConPrint(L"\n");
    }
    */
    
    //Load Module-Settings
    Modules::LoadModules();

    //SQL-Module ################################################################################
    if (Modules::GetModuleState("SQLModule"))
    {
        SQL::InitializeDB();

        ConPrint(L"Module loaded: SQL\n");

        //PVP::UpdateDuelRanking(L"test", true);

    }
	
    // CARRIER-Module ###########################################################################
    if (Modules::GetModuleState("CarrierModule"))
    {
        //Read Settings
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scCarrier = std::string(szCurDir) + CARRIER_CONFIG_FILE;
        
        //Lade CarrierIDs
        Docking::lCarrierConfig.clear();
        for (int i = 0;; i++) {
            char szBuf[64];
            sprintf(szBuf, "Carrier%u", i);
            std::string CarrierNickname = IniGetS(scCarrier, "Carrier", szBuf, "");

            if (CarrierNickname == "")
                break;

            // CarrierKonfig in die Liste
            Docking::CarrierConfig NewCarrierConfig;
            NewCarrierConfig.iShipArch = CreateID(CarrierNickname.c_str());
            NewCarrierConfig.iSlots = IniGetI(scCarrier, "Carrierslots", CarrierNickname, 5);
            NewCarrierConfig.sInterior = IniGetS(scCarrier, "Carrierinterior", CarrierNickname, "Li_Proxy_Li_Battleship_Base");
			NewCarrierConfig.fx_Undock = IniGetF(scCarrier, "CarrierOffset", CarrierNickname + "-y", 0.0f);
			NewCarrierConfig.fy_Undock = IniGetF(scCarrier, "CarrierOffset", CarrierNickname + "-z", 0.0f);
            NewCarrierConfig.fz_Undock = IniGetF(scCarrier, "CarrierOffset", CarrierNickname + "-x", 0.0f);

            //ConPrint(stows(CarrierNickname) + L"\n");
            //ConPrint(std::to_wstring(NewCarrierConfig.iShipArch) + L"\n");
            //ConPrint(std::to_wstring(NewCarrierConfig.iSlots) + L"\n");


            Docking::lCarrierConfig.push_back(NewCarrierConfig);
        }

        //LadeCarrier Request Timeout
        Docking::msRequestTimeout = IniGetI(scCarrier, "CarrierModule", "DockRequestTimeout", 10000);
        
        Docking::fDockRange = IniGetF(scCarrier, "CarrierModule", "DockRange", 200.0f);


        ConPrint(L"Module loaded: Carrier (" + stows(std::to_string(Docking::lCarrierConfig.size())) + L" Carriers loaded)\n");
    }

    // POPUP-Module #############################################################################
    if (Modules::GetModuleState("WelcomeMSG"))
    {
        //Welcome Message - First Char on ID
        PopUp::iWMsg_Head = IniGetI(scPluginCfgFile, "WelcomePopUp", "Head", 520002);
        PopUp::iWMsg_Body = IniGetI(scPluginCfgFile, "WelcomePopUp", "Body", 520003);

        ConPrint(L"Module loaded: WelcomeMSG\n");
    }

    //Load always Contributor 
    PopUp::iContributor_Head = IniGetI(scPluginCfgFile, "ContributorPopUp", "Head", 520000);
    PopUp::iContributor_Body = IniGetI(scPluginCfgFile, "ContributorPopUp", "Body", 520001);

    // INSURANCE-Module #########################################################################
    if (Modules::GetModuleState("InsuranceModule"))
    {
        Insurance::set_fCostPercent = IniGetF(scPluginCfgFile, "InsuranceModule", "CostPercent", 100.0f);

        ConPrint(L"Module loaded: Insurance (CostPercent: " + std::to_wstring(Insurance::set_fCostPercent) + L")\n");
    }

    // ANTICHEAT-Module   ########################################################################
    CRCAntiCheat_FLSR = (_CRCAntiCheat)((char *)hModServer + ADDR_CRCANTICHEAT);
    if (Modules::GetModuleState("ACModule"))
    {
        struct PlayerData* pPD = 0;
        while (pPD = Players.traverse_active(pPD))
        {
            uint iClientID = HkGetClientIdFromPD(pPD);
            AntiCheat::SpeedAC::Init(iClientID);
            AntiCheat::TimingAC::Init(iClientID);
            AntiCheat::PowerAC::Init(iClientID);
        }
        ConPrint(L"Module loaded: AC\n");
    }

    //CUSTOMMISSION-Module   ###################################################################
    if (Modules::GetModuleState("CustomMissionModule"))
    {
        //Load CustomMissions
        //CustomMissions::LoadMissions();
    }

	//EquipWhiteList-Module ######################################################################
    if (Modules::GetModuleState("EquipWhiteListModule"))
    {
        EquipWhiteList::LoadEquipWhiteList();

        ConPrint(L"Module loaded: EquipWhiteList (" + stows(std::to_string(EquipWhiteList::lEquipWhiteList.size())) + L" entries loaded)\n");
    }

	//CLOAK-Module ##############################################################################
    if (Modules::GetModuleState("CloakModule"))
    {
        Cloak::LoadCloakSettings();

        ConPrint(L"Module loaded: Cloak (" + stows(std::to_string(Cloak::lCloakDeviceList.size())) + L" Devices loaded)\n");

    }

    //DEPOT-Module ##############################################################################
    if (Modules::GetModuleState("DepotModule"))
    {
        if (Modules::GetModuleState("SQLModule"))
        {
            if (Depot::LoadDepotData())
            {
                ConPrint(L"Module loaded: Depot (" + stows(std::to_string(Depot::lPlayerDepot.size())) + L" entries loaded)\n");                
            }
            else {
                ConPrint(L"Module failed to load: DepotModule\n");
                Modules::SwitchModuleState("DepotModule");
            }

        }
        else
        {
            ConPrint(L"Depot Module not loaded! Necessary Module not loaded: SQL\n");
        }
    }

    //CMPDUMPS     #############################################################################
    if (Modules::GetModuleState("UpdateCMP"))
    {
        //Get Exceptions
        Tools::get_cmpExceptions();
        
        //Update CMPFiles
        Tools::get_cmpfiles(DATADIR + std::string("\\SHIPS"));
		ConPrint(L"Module loaded: CMPUpdate with " + std::to_wstring(Tools::lCMPUpdateExceptions.size()) + L" Exceptions\n");
    }
    
    //PathSelection     #############################################################################
    if (Modules::GetModuleState("PathSelection"))
    {
		//Load PathSelection
        PathSelection::LoadPathSelectionSettings();
        ConPrint(L"Module loaded: PathSelection with " + std::to_wstring(PathSelection::lBlockedGates.size()) + L" BlockedGates and " + std::to_wstring(PathSelection::lReputations.size()) + L" Reputations\n");
    }


    //PlayerHunt     #############################################################################
    if (Modules::GetModuleState("PlayerHunt"))
    {
		//Load PlayerHunt
		PlayerHunt::LoadPlayerHuntSettings();
		ConPrint(L"Module loaded: PlayerHunt - RewardMultiplicator: " + std::to_wstring(PlayerHunt::set_fRewardMultiplicator) + L", MinTargetSystemDistance: " + std::to_wstring(PlayerHunt::set_iMinTargetSystemDistance) + L", MinCredits: " + std::to_wstring(PlayerHunt::set_iMinCredits) + L"\n");
	}

    //PVP     #############################################################################
    if (Modules::GetModuleState("PVP"))
    {
        //Load PVP
        //PlayerHunt::LoadPlayerHuntSettings();
        ConPrint(L"Module loaded: PVP \n");
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

    return true;

}

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode() {
    return returncode;
}

EXPORT PLUGIN_INFO *Get_PluginInfo() {
    PLUGIN_INFO *p_PI = new PLUGIN_INFO();
    p_PI->sName = "FL:SR Server Plugin";
    p_PI->sShortName = "ServerPlugin";
    p_PI->bMayPause = false;
    p_PI->bMayUnload = true;
    p_PI->ePluginReturnCode = &returncode;
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Timers::Update,PLUGIN_HkIServerImpl_Update, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&LoadSettings, PLUGIN_LoadSettings, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::LaunchComplete, PLUGIN_HkIServerImpl_LaunchComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::ShipDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Commands::UserCmd_Process, PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Commands::ExecuteCommandString_Callback,PLUGIN_ExecuteCommandString_Callback, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Commands::CmdHelp_Callback, PLUGIN_CmdHelp_Callback, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::HkCb_AddDmgEntry, PLUGIN_HkCb_AddDmgEntry, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SPObjUpdate,PLUGIN_HkIServerImpl_SPObjUpdate, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SubmitChat,PLUGIN_HkIServerImpl_SubmitChat, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::Dock_Call, PLUGIN_HkCb_Dock_Call, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SPMunitionCollision, PLUGIN_HkIServerImpl_SPMunitionCollision, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::JumpInComplete, PLUGIN_HkIServerImpl_JumpInComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SystemSwitchOutComplete, PLUGIN_HkIServerImpl_SystemSwitchOutComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::ClearClientInfo, PLUGIN_ClearClientInfo, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::FireWeapon, PLUGIN_HkIServerImpl_FireWeapon, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::PlayerLaunch_After, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::ReqAddItem, PLUGIN_HkIServerImpl_ReqAddItem, 0)); 
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::ReqShipArch_AFTER, PLUGIN_HkIServerImpl_ReqShipArch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::ReqEquipment, PLUGIN_HkIServerImpl_ReqEquipment, 0)); 
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::GoTradelane, PLUGIN_HkIServerImpl_GoTradelane, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::CreateNewCharacter_After, PLUGIN_HkIServerImpl_CreateNewCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::RequestEvent, PLUGIN_HkIServerImpl_RequestEvent, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::PopUpDialog, PLUGIN_HkIServerImpl_PopUpDialog, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SendDeathMsg, PLUGIN_SendDeathMsg, 0));



    return p_PI;
}
