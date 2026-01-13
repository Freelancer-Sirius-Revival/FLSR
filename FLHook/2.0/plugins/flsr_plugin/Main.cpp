#include "Main.h"
#include "Pilots.h"
#include "Empathies.h"
#include "GroupRep.h"
#include "NpcCloaking.h"
#include "NameLimiter.h"
#include "Missions/ShipSpawning.h"
#include "Missions/Formations.h"
#include "Missions/NpcAppearances.h"
#include "Missions/BestPath.h"
#include "Missions/ClientObjectives.h"
#include "Missions/LifeTimes.h"
#include "Missions/Missions.h"
#include "Missions/MissionBoard.h"
#include "Missions/Mission.h"
#include "Missions/conditions/CndBaseEnter.h"
#include "Missions/conditions/CndBaseExit.h"
#include "Missions/conditions/CndCloaked.h"
#include "Missions/conditions/CndCommComplete.h"
#include "Missions/conditions/CndDestroyed.h"
#include "Missions/conditions/CndDistObj.h"
#include "Missions/conditions/CndDistVec.h"
#include "Missions/conditions/CndHasCargo.h"
#include "Missions/conditions/CndHealthDec.h"
#include "Missions/conditions/CndHealthInc.h"
#include "Missions/conditions/CndInSpace.h"
#include "Missions/conditions/CndInSystem.h"
#include "Missions/conditions/CndInZone.h"
#include "Missions/conditions/CndJumpInComplete.h"
#include "Missions/conditions/CndLaunchComplete.h"
#include "Missions/conditions/CndOnBase.h"
#include "Missions/conditions/CndProjHitCount.h"
#include "Missions/conditions/CndSystemSpaceEnter.h"
#include "Missions/conditions/CndSystemSpaceExit.h"
#include "Missions/conditions/CndTimer.h"
#include "Missions/objectives/Objectives.h"
#include "MissionAbortFix.h"
#include "PlayerLootSpawning.h"
#include "ExplosionDamage.h"

std::mutex m_Mutex;

PLUGIN_RETURNCODE returncode;


void LoadSettings() {
    Pilots::ReadFiles();
    NpcAppearances::ReadFiles();
    Formations::ReadFiles();
    BestPath::ReadFiles();
    NpcCloaking::ReadFiles();

    // Konfigpfad
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    std::string scPluginCfgFile = std::string(szCurDir) + Globals::PLUGIN_CONFIG_FILE;

    ConnectionLimiter::maxParallelConnectionsPerIpAddress = IniGetI(scPluginCfgFile, "ConnectionLimiter", "MaxParallelIpAddressConnections", 2);
    
    //Load Module-Settings
    Modules::LoadModules();

    if (Modules::GetModuleState("CarrierModule"))
    {
        ConPrint(L"Module loaded: Carrier\n");
        Docking::LoadSettings();
    }

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


    EquipWhiteList::LoadEquipWhiteList();
    ConPrint(L"Module loaded: EquipWhiteList\n");

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
    returncode = DEFAULT_RETURNCODE;
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

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::Initialize, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::Send_FLPACKET_SERVER_LAUNCH, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_LAUNCH, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::PlayerLaunch_After, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::Dock_Call_After, PLUGIN_HkCb_Dock_Call_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::SolarDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&SolarSpawn::ExecuteCommandString, PLUGIN_ExecuteCommandString_Callback, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ReadCharacterData, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::Send_FLPACKET_SERVER_CREATESHIP_AFTER, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESHIP_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipEquipDamage, PLUGIN_ShipEquipDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipColGrpDamage, PLUGIN_ShipColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipHullDamage, PLUGIN_ShipHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&IFF::ShipShieldDamage, PLUGIN_ShipShieldDmg, 0));
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
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::GuidedInit, PLUGIN_HkIEngine_CGuided_init, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Cloak::ActivateCruise, PLUGIN_HkIServerImpl_ActivateCruise, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NpcCloaking::ShipAndSolarDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NpcCloaking::ShipAndSolarEquipDestroyedHook, PLUGIN_ShipEquipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NpcCloaking::ShipAndSolarDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NpcCloaking::ShipAndSolarEquipDestroyedHook, PLUGIN_SolarEquipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NpcCloaking::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NpcCloaking::Send_FLPACKET_SERVER_CREATESHIP_AFTER, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESHIP_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NpcCloaking::Send_FLPACKET_SERVER_CREATESOLAR_AFTER, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESOLAR_AFTER, 0));

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
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::GFGoodBuy, PLUGIN_HkIServerImpl_GFGoodBuy, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::ReqEquipment, PLUGIN_HkIServerImpl_ReqEquipment, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&EquipWhiteList::ReqAddItem, PLUGIN_HkIServerImpl_ReqAddItem, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&BatsBotsShipTransferFix::GFGoodBuy, PLUGIN_HkIServerImpl_GFGoodBuy, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&BatsBotsShipTransferFix::ReqEquipment, PLUGIN_HkIServerImpl_ReqEquipment, 0));

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
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::PlayerLaunch_After, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Commands::UserCmd_Process, PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::PopUpDialog, PLUGIN_HkIServerImpl_PopUpDialog, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Hooks::SendDeathMsg, PLUGIN_SendDeathMsg, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Empathies::Initialize, PLUGIN_HkTimerCheckKick, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&GroupReputation::ObjectDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&GroupReputation::ObjectDestroyed, PLUGIN_ShipDestroyed, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Crafting::UserCmd_Craft, PLUGIN_UserCmd_Process, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&LootBoxes::UserCmd_Open, PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&LootBoxes::ReadInitialData, PLUGIN_HkTimerCheckKick, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Storage::InitializeStorageSystem, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Storage::UserCmd_Storage, PLUGIN_UserCmd_Process, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&ConnectionLimiter::Login_After, PLUGIN_HkIServerImpl_Login_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&ConnectionLimiter::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&ShipSpawning::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&ShipSpawning::SystemSwitchOutComplete_AFTER, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&PlayerLootSpawning::ShipDestroyed, PLUGIN_ShipDestroyed, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&NameLimiter::CreateNewCharacter, PLUGIN_HkIServerImpl_CreateNewCharacter, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&ExplosionDamage::ExplosionHit, PLUGIN_ShipExplosionHit, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&ExplosionDamage::ExplosionHit, PLUGIN_SolarExplosionHit, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::ObjCndNpcSimulationRunning::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::ObjCndNpcSimulationRunning::SystemSwitchOutComplete_AFTER, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndBaseEnter::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndBaseExit::BaseExit_AFTER, PLUGIN_HkIServerImpl_BaseExit_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndCloaked::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndCommComplete::CommComplete, PLUGIN_HkIServerImpl_CommComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndDestroyed::ObjDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndDestroyed::ObjDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndDistObj::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndDistVec::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHasCargo::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHasCargo::ReqAddItem_AFTER, PLUGIN_HkIServerImpl_ReqAddItem_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHasCargo::ReqEquipment_AFTER, PLUGIN_HkIServerImpl_ReqEquipment_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthDec::ShipAndSolarColGrpDamage, PLUGIN_ShipColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthDec::ShipAndSolarHullDamage, PLUGIN_ShipHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthDec::ShipAndSolarColGrpDamage, PLUGIN_SolarColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthDec::ShipAndSolarHullDamage, PLUGIN_SolarHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthInc::ShipAndSolarColGrpDamage, PLUGIN_ShipColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthInc::ShipAndSolarHullDamage, PLUGIN_ShipHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthInc::ShipAndSolarColGrpDamage, PLUGIN_SolarColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndHealthInc::ShipAndSolarHullDamage, PLUGIN_SolarHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndInSpace::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndInSpace::SystemSwitchOutComplete_AFTER, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndInSystem::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndInSystem::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndInSystem::SystemSwitchOutComplete_AFTER, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndInZone::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndJumpInComplete::JumpInComplete, PLUGIN_HkIServerImpl_JumpInComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndLaunchComplete::LaunchComplete_AFTER, PLUGIN_HkIServerImpl_LaunchComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndOnBase::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarEquipDamage, PLUGIN_ShipEquipDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarColGrpDamage, PLUGIN_ShipColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarHullDamage, PLUGIN_ShipHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarShieldDamage, PLUGIN_ShipShieldDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarEquipDamage, PLUGIN_SolarEquipDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarColGrpDamage, PLUGIN_SolarColGrpDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarHullDamage, PLUGIN_SolarHullDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndProjHitCount::ShipAndSolarShieldDamage, PLUGIN_SolarShieldDmg, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceEnter::BaseExit, PLUGIN_HkIServerImpl_BaseExit, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceEnter::CharacterInfoReq, PLUGIN_HkIServerImpl_CharacterInfoReq, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceEnter::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceEnter::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceEnter::SystemSwitchOutComplete_AFTER, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceExit::BaseEnter, PLUGIN_HkIServerImpl_BaseEnter, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceExit::SystemSwitchOutComplete, PLUGIN_HkIServerImpl_SystemSwitchOutComplete, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndSystemSpaceExit::ObjDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::CndTimer::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::Mission::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::Mission::Dock_Call, PLUGIN_HkCb_Dock_Call, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Hooks::LifeTimes::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));

    // Must be after the mission conditions to prevent de-registering any stuff that may be required for the conditions to still exist on missions.
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Initialize, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::Shutdown, PLUGIN_HkIServerImpl_Shutdown, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ObjDestroyed, PLUGIN_SolarDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ObjDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ExecuteCommandString, PLUGIN_ExecuteCommandString_Callback, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ClientObjectives::Send_FLPACKET_COMMON_REQUEST_BEST_PATH, PLUGIN_HkIClientImpl_Send_FLPACKET_COMMON_REQUEST_BEST_PATH, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ClientObjectives::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ClientObjectives::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ClientObjectives::GoTradelane_AFTER, PLUGIN_HkIServerImpl_GoTradelane_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ClientObjectives::StopTradelane_AFTER, PLUGIN_HkIServerImpl_StopTradelane_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&Missions::ClientObjectives::Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::Initialize, PLUGIN_HkTimerCheckKick, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::MissionResponse, PLUGIN_HkIServerImpl_MissionResponse, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::AbortMission, PLUGIN_HkIServerImpl_AbortMission, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_GFUPDATEMISSIONCOMPUTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::Send_FLPACKET_SERVER_GFCOMPLETEMISSIONCOMPUTERLIST, PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_GFCOMPLETEMISSIONCOMPUTERLIST, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::BaseEnter, PLUGIN_HkIServerImpl_BaseEnter, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionBoard::BaseExit, PLUGIN_HkIServerImpl_BaseExit, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionAbortFix::CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionAbortFix::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&MissionAbortFix::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));

    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&BestPath::CollectJumpObjectsPerSystem, PLUGIN_HkTimerCheckKick, 0));
    
    return p_PI;
}
