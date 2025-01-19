// includes
#include "Main.h"

// Mutex-Objekt definieren
std::mutex m_Mutex;

PLUGIN_RETURNCODE returncode;
_CRCAntiCheat CRCAntiCheat_FLSR;


/// Lade Konfig
void LoadSettings() {
    returncode = DEFAULT_RETURNCODE;
    // Konfigpfad
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    std::string scPluginCfgFile = std::string(szCurDir) + Globals::PLUGIN_CONFIG_FILE;

    //Load HashMap
    if (Tools::ReadIniNicknames())
    {
        ConPrint(L"\n----------LOADING FLSR-HashMap----------\n");
        ConPrint(L"Loaded HashMap with %d entries\n", Tools::mNicknameHashMap.size());
        ConPrint(L"\n");
    }
        
    //Load Module-Settings
    Modules::LoadModules();

    //SQL-Module ################################################################################
    if (Modules::GetModuleState("SQLModule"))
    {
        SQL::InitializeDB();

        ConPrint(L"Module loaded: SQL\n");
    }

    if (Modules::GetModuleState("CarrierModule"))
    {
        ConPrint(L"Module loaded: Carrier\n");
        Docking::LoadSettings();
    }

    SolarInvincibility::LoadSettings();
    SolarSpawn::LoadSettings();

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
        Insurance::insuranceEquipmentCostFactor = std::max(0.0f, IniGetF(scPluginCfgFile, "InsuranceModule", "CostPercent", 100.0f) / 100.0f);
        ConPrint(L"Module loaded: Insurance (CostFactor: " + std::to_wstring(Insurance::insuranceEquipmentCostFactor) + L")\n");
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

	//EquipWhiteList-Module ######################################################################
    if (Modules::GetModuleState("EquipWhiteListModule"))
    {
        EquipWhiteList::LoadEquipWhiteList();

        ConPrint(L"Module loaded: EquipWhiteList\n");
    }

	//CLOAK-Module ##############################################################################
    if (Modules::GetModuleState("CloakModule"))
    {
        Cloak::LoadCloakSettings();

        ConPrint(L"Module loaded: Cloak\n");

    }

    //CMPDUMPS     #############################################################################
    if (Modules::GetModuleState("UpdateCMP"))
    {
        //Get Exceptions
        Tools::get_cmpExceptions();
        
        //Update CMPFiles
        Tools::get_cmpfiles(Globals::DATADIR + std::string("\\SHIPS"));
		ConPrint(L"Module loaded: CMPUpdate with " + std::to_wstring(Tools::lCMPUpdateExceptions.size()) + L" Exceptions\n");
    }


    //PlayerHunt     #############################################################################
    if (Modules::GetModuleState("PlayerHunt"))
    {
		//Load PlayerHunt
		PlayerHunt::LoadPlayerHuntSettings();
		ConPrint(L"Module loaded: PlayerHunt - RewardMultiplicator: " + std::to_wstring(PlayerHunt::set_fRewardMultiplicator) + L", MinTargetSystemDistance: " + std::to_wstring(PlayerHunt::set_iMinTargetSystemDistance) + L", MinCredits: " + std::to_wstring(PlayerHunt::set_iMinCredits) + L"\n");
	}

    SpawnProtection::LoadSettings();

    if (Modules::GetModuleState("Crafting"))
    {
        Crafting::LoadSettings();
        ConPrint(L"Module loaded: Crafting\n");
    }


    //DiscordBot     #############################################################################
    if (Modules::GetModuleState("DiscordBot"))
    {
        //Load DiscordBot
        if (Discord::LoadSettings())
        {           
            try {
                HANDLE hDiscordBotThread;
          
                DWORD id;
                DWORD dwParam;
                hDiscordBotThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Discord::StartUp, &dwParam, 0, &id);
                
                //CleanUp CharManager
                Discord::CharManager_DeleteInvalidEntries();


                ConPrint(L"Module loaded: DiscordBot\n");

            }
            catch (const char* e) {
				ConPrint(L"Module failed to load: DiscordBot\n");
				Modules::SetModuleState("DiscordBot", false);
			}


        }
        else
        {
            ConPrint(L"Module failed to load: DiscordBot\n");
            Modules::SetModuleState("DiscordBot", false);
        }

    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return true;
}

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
    return returncode;
}

EXPORT PLUGIN_INFO *Get_PluginInfo()
{
    PLUGIN_INFO *p_PI = new PLUGIN_INFO();
    p_PI->sName = "FL:SR Server Plugin";
    p_PI->sShortName = "ServerPlugin";
    p_PI->bMayPause = false;
    p_PI->bMayUnload = true;
    p_PI->ePluginReturnCode = &returncode;
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Timers::Update,PLUGIN_HkIServerImpl_Update, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarInvincibility::Initialize, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::Initialize, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::SolarDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::ExecuteCommandString, PLUGIN_ExecuteCommandString_Callback, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ReadCharacterData, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::Send_FLPACKET_SERVER_CREATESHIP_AFTER, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESHIP_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipEquipDamage, PLUGIN_ShipEquipDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipShieldDamage, PLUGIN_ShipShieldDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipColGrpDamage, PLUGIN_ShipColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipHullDamage, PLUGIN_ShipHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::CreateNewCharacter_After, PLUGIN_HkIServerImpl_CreateNewCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::DestroyCharacter_After, PLUGIN_HkIServerImpl_DestroyCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::HkRename, PLUGIN_HkCb_Rename, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::HkRename_After, PLUGIN_HkCb_Rename_AFTER, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::InitializeWithGameData, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::ActivateEquip, PLUGIN_HkIServerImpl_ActivateEquip, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::JumpInComplete, PLUGIN_HkIServerImpl_JumpInComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::PlayerLaunch_After, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::GoTradelane, PLUGIN_HkIServerImpl_GoTradelane, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::Dock_Call, PLUGIN_HkCb_Dock_Call, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::BaseEnter, PLUGIN_HkIServerImpl_BaseEnter, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::BaseExit, PLUGIN_HkIServerImpl_BaseExit, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::SPObjUpdate, PLUGIN_HkIServerImpl_SPObjUpdate, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::ShipEquipDestroyed, PLUGIN_ShipEquipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::SolarDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::ShipDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::ActivateCruise, PLUGIN_HkIServerImpl_ActivateCruise, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::SystemSwitchOutComplete_After, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::PlayerLaunch_After, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::BaseEnter_After, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::Send_FLPACKET_SERVER_CREATESHIP_AFTER, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESHIP_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::SolarDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::ShipDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::DestroyCharacter_After, PLUGIN_HkIServerImpl_DestroyCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::HkRename, PLUGIN_HkCb_Rename, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Mark::HkRename_After, PLUGIN_HkCb_Rename_AFTER, 0));
    
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::BaseExit, PLUGIN_HkIServerImpl_BaseExit, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::ReqAddItem_AFTER, PLUGIN_HkIServerImpl_ReqAddItem_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::ReqEquipment_AFTER, PLUGIN_HkIServerImpl_ReqEquipment_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::ReqShipArch_AFTER, PLUGIN_HkIServerImpl_ReqShipArch_AFTER, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::InitializeWithGameData, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::ReqShipArch_After, PLUGIN_HkIServerImpl_ReqShipArch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::LaunchComplete_After, PLUGIN_HkIServerImpl_LaunchComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::JumpInComplete_After, PLUGIN_HkIServerImpl_JumpInComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::Dock_Call, PLUGIN_HkCb_Dock_Call, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::PlayerLaunch_After, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::BaseEnter_After, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::CreateNewCharacter_After, PLUGIN_HkIServerImpl_CreateNewCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::DestroyCharacter_After, PLUGIN_HkIServerImpl_DestroyCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::HkRename, PLUGIN_HkCb_Rename, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Docking::HkRename_After, PLUGIN_HkCb_Rename_AFTER, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SpawnProtection::LaunchComplete_AFTER, PLUGIN_HkIServerImpl_LaunchComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SpawnProtection::JumpInComplete_AFTER, PLUGIN_HkIServerImpl_JumpInComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SpawnProtection::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SpawnProtection::SystemSwitchOutComplete_AFTER, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Insurance::CreateNewCharacter_After, PLUGIN_HkIServerImpl_CreateNewCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Insurance::DestroyCharacter_After, PLUGIN_HkIServerImpl_DestroyCharacter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Insurance::HkRename, PLUGIN_HkCb_Rename, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Insurance::HkRename_After, PLUGIN_HkCb_Rename_AFTER, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&LoadSettings, PLUGIN_LoadSettings, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::LaunchComplete, PLUGIN_HkIServerImpl_LaunchComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Commands::UserCmd_Process, PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Commands::ExecuteCommandString_Callback,PLUGIN_ExecuteCommandString_Callback, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SPObjUpdate,PLUGIN_HkIServerImpl_SPObjUpdate, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::Dock_Call, PLUGIN_HkCb_Dock_Call, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SPMunitionCollision, PLUGIN_HkIServerImpl_SPMunitionCollision, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::JumpInComplete, PLUGIN_HkIServerImpl_JumpInComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SystemSwitchOutComplete, PLUGIN_HkIServerImpl_SystemSwitchOutComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::ClearClientInfo, PLUGIN_ClearClientInfo, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::FireWeapon, PLUGIN_HkIServerImpl_FireWeapon, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::PlayerLaunch_After, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::PopUpDialog, PLUGIN_HkIServerImpl_PopUpDialog, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SendDeathMsg, PLUGIN_SendDeathMsg, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&GroupReputation::InitializeWithGameData, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&GroupReputation::SolarDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&GroupReputation::ShipDestroyed, PLUGIN_ShipDestroyed, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Crafting::UserCmd_Craft, PLUGIN_UserCmd_Process, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&LootBoxes::UserCmd_Open, PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&LootBoxes::ReadInitialData, PLUGIN_HkTimerCheckKick, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Storage::InitializeStorageSystem, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Storage::UserCmd_Storage, PLUGIN_UserCmd_Process, 0));

    return p_PI;
}
