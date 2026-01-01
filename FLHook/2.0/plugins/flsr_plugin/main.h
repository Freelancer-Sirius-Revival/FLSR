#pragma once
#ifndef __MAIN_H__
#define __MAIN_H__ 1

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

//Disable Warnings
#pragma warning( disable : 4251 ) // needs to have dll-interface to be used by clients of class

//DISCORD Includes
#include <dpp/dpp.h>

//FLHOOK Includes
#include <FLHook.h>
#include "Plugin.h"

//Includes
#include <list>
#include <string>
#include <Windows.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <numeric>
#include <unordered_map>
#include <openssl/sha.h>

//Offsets
#define ADDR_CLIENT_NEWPLAYER 0x8010

// Mutex-Objekt deklarieren
extern std::mutex m_Mutex;


//Namespaces

namespace Globals {

    //FilePaths
    const std::string PLUGIN_CONFIG_FILE = "\\flhook_plugins\\flsr.cfg";
    const std::string CARRIER_CONFIG_FILE = "\\flhook_plugins\\FLSR-Carrier.cfg";
    const std::string CLOAK_CONFIG_FILE = "\\flhook_plugins\\FLSR-Cloak.cfg";
    const std::string CRAFTING_CONFIG_FILE = "\\flhook_plugins\\FLSR-Crafting.cfg";
    const std::string LOOTBOXES_CONFIG_FILE = "\\flhook_plugins\\FLSR-LootBoxes.cfg";
    const std::string SENDCASHLOG_FILE = "-givecashlog.txt";
    const std::string LIBRELANCER_SDK = "\\flhook_plugins\\librelancer-sdk\\";
    const std::string CMP_DUMP_FOLDER = "\\flhook_plugins\\flsr-cmpdumps\\";
    const std::string Equip_WHITELIST_FILE = "\\flhook_plugins\\FLSR-EquipWhiteList.cfg";
    const std::string DATADIR = "..\\DATA";
}

namespace Modules {
    struct Module {
        std::string scModuleName;
        bool bModulestate;
    };

    void LoadModules();
    bool GetModuleState(std::string scModuleName);
    bool SetModuleState(const std::string& scModuleName, bool bModuleState);
	
    extern std::list<Module> lModules;
}

namespace Timers {
	
    typedef void (*_TimerFunc)();

    struct TIMER {
        _TimerFunc proc;
        mstime tmIntervallMS;
        mstime tmLastCall;
    };

    int __stdcall Update();

}

namespace Commands {
    void UserCMD_Contributor(uint iClientID, const std::wstring &wscParam);
    bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd);
    void UserCMD_SendCash$(uint iClientID, const std::wstring &wscParam);
    void UserCMD_SendCash(uint iClientID, const std::wstring &wscParam);
    void UserCmd_UV(uint iClientID, const std::wstring &wscParam);
    void UserCmd_HELP(uint iClientID, const std::wstring& wscParam);
    
    typedef void (*_UserCmdProc)(uint, const std::wstring &);
    struct USERCMD {
        const  wchar_t* wszCmd;
        _UserCmdProc proc;
    };

    #define IS_CMD(a) !wscCmd.compare(L##a)
}

namespace SendCash {
    static int set_iMinTime = 0;
    static int set_iMinTransfer = 0;

    void LogTransfer(std::wstring wscToCharname, std::wstring msg);
}

namespace Chat {
    HK_ERROR HkSendUChat(std::wstring wscCharname, std::wstring wscText);
}

namespace PopUp {
    // Contributor
    extern uint iContributor_Head;
    extern uint iContributor_Body;

    // Welcome Message - First Char on ID
    extern uint iWMsg_Head;
    extern uint iWMsg_Body;

    void WelcomeBox(uint iClientID);
    void OpenPopUp(uint iClientID);
    void HandleButtonClick(uint iClientID, uint buttonClicked);

    enum PopUpType {
		PopUpType_Help
	};

    //POPUPBox with Handle
    struct PopUpBox {
        uint iClientID;
        uint iHead;
        uint iBody;
        uint iPage;
		uint iMaxPage;
        uint iButton;
		PopUpType iType;
    };
	
    extern std::map<std::wstring, PopUpBox> mPopUpBox;
    //extern std::list<PopUpBox> lPOPUPBox;
}


namespace Tools {

    enum eDeathTypes {
        PVP,
        SUICIDE,
        PVE,
        KILLEDHIMSELF,
        ADMIN,
        HASDIED
    };

    struct HashMap {
        std::string scNickname;
        uint iResID;
    };

	struct CMPDump_Exception {
		std::string scData;
	};

    struct CMPDump_Entry {
        bool bisCollGroup;
		std::string scData;
        bool bhasParent;
		std::string scParent;
        
    };
    void get_cmpExceptions();
    void get_cmpfiles(const std::filesystem::path& path);
    extern std::list <CMPDump_Exception> lCMPUpdateExceptions;
    void HkNewPlayerMessage(uint iClientID, struct CHARACTER_ID const &cId);
 
}

namespace Docking
{
    void LoadSettings();
    void InitializeWithGameData();
    void __stdcall LaunchComplete_After(unsigned int baseId, unsigned int shipId);
    void __stdcall JumpInComplete_After(unsigned int systemId, unsigned int shipId);
    int __cdecl Dock_Call(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response);
    void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId);
    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
    void __stdcall ReqShipArch_After(unsigned int archetypeId, unsigned int clientId);
    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    void UserCmd_Dock(const uint clientId, const std::wstring& wscParam);
}

namespace Hooks {
    void __stdcall PopUpDialog(unsigned int iClientID, unsigned int buttonClicked);
    void __stdcall CharacterSelect(struct CHARACTER_ID const &cId, unsigned int iClientID);
    void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip);
    void SendDeathMsg(const std::wstring& wscMsg, uint iSystemID, uint iClientIDVictim, uint iClientIDKiller);
    }

namespace IFF
{
    void ReadCharacterData();
    void UserCmd_Hostile(const uint clientId, const std::wstring& arguments);
    void UserCmd_Neutral(const uint clientId, const std::wstring& arguments);
    void UserCmd_Allied(const uint clientId, const std::wstring& arguments);
    void UserCmd_Attitude(const uint clientId, const std::wstring& arguments);
    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship);
    void __stdcall ShipEquipDamage(const IObjRW* damagedObject, const CEquip* hitEquip, const float& incomingDamage, const DamageList* damageList);
    void __stdcall ShipShieldDamage(const IObjRW* damagedObject, const CEShield* hitShield, const float& incomingDamage, const DamageList* damageList);
    void __stdcall ShipColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList);
    void __stdcall ShipHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList);
    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
}

namespace Cloak
{
    enum class CloakState
    {
        Uncloaked,
        Cloaking,
        Cloaked,
        Uncloaking
    };

    void LoadCloakSettings();
    void InitializeWithGameData();
    void UpdateCloakClients();
    CloakState GetClientCloakState(uint clientId);
    bool TryRegisterNoCloakSolar(const std::string& nickname, uint objectId);
    extern const uint TIMER_INTERVAL;
    void __stdcall ActivateEquip(unsigned int clientId, XActivateEquip const& activateEquip);
    void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId);
    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
    void __stdcall GoTradelane(unsigned int clientId, struct XGoTradelane const& goToTradelane);
    int __cdecl Dock_Call(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response);
    void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId);
    void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
    void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
    void __stdcall SPObjUpdate(SSPObjUpdateInfo const& updateInfo, unsigned int clientId);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state);
    void __stdcall ShipEquipDestroyed(const IObjRW* object, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* damageList);
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void GuidedInit(CGuided* guided, CGuided::CreateParms& parms);
    void __stdcall ActivateCruise(unsigned int clientId, struct XActivateCruise const& activateCruise);
    void UserCmd_CLOAK(uint clientId, const std::wstring& wscParam);
    void UserCmd_UNCLOAK(uint clientId, const std::wstring& wscParam);
}

namespace Crafting
{
    void LoadSettings();
    bool UserCmd_Craft(const uint clientId, const std::wstring& argumentsWS);
}

namespace LootBoxes
{
    void ReadInitialData();
    bool UserCmd_Open(const uint clientId, const std::wstring& argumentsWS);
}

namespace Mark
{
    extern const uint CLEAR_ROTATION_TIMER_INTERVAL;
    bool MarkObject(const uint clientId, const uint targetId);
    bool UnmarkObject(const uint clientId, const uint targetId);
    void AddCloakedPlayer(const uint clientId);
    void RemoveCloakedPlayer(const uint clientId);
    void RotateClearNonExistingTargetIds();
    void UserCmd_Mark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_GroupMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnGroupMark(const uint clientId, const std::wstring& wscParam);
    void UserCmd_UnMarkAll(const uint clientId, const std::wstring& wscParam);
    void __stdcall SystemSwitchOutComplete_After(unsigned int shipId, unsigned int clientId);
    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId);
    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection state);
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
}

namespace SolarSpawn
{
    struct SolarArchetype
    {
        bool autospawn = false;
        uint archetypeId = 0;
        uint loadoutId = 0;
        std::string nickname = "";
        uint nicknameCounter = 0;
        uint idsName = 0;
        uint idsInfocard = 0;
        Vector position;
        Matrix orientation;
        uint systemId = 0;
        uint baseId = 0;
        std::string affiliation = "";
        uint pilotId = 0;
        int hitpoints = -1;
        uint voiceId = 0;
        uint headId = 0;
        uint bodyId = 0;
        std::vector<uint> accessoryIds = {};
    };

    void LoadSettings();
    void Initialize();
    void AppendSolarArchetype(const SolarArchetype& archetype);
    bool __stdcall Send_FLPACKET_SERVER_LAUNCH(uint iClientID, FLPACKET_LAUNCH& pLaunch);
    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId);
    int __cdecl Dock_Call_After(unsigned int const& ship, unsigned int const& dockTargetId, int dockPortIndex, DOCK_HOST_RESPONSE response);
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd);
    uint SpawnSolarByName(std::string name);
    bool DestroySolar(const uint spaceObjId, const DestroyType destroyType);
}

namespace EquipWhiteList
{
    void LoadEquipWhiteList();
    void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
    void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
    void __stdcall GFGoodBuy(const SGFGoodBuyInfo& gbi, unsigned int clientId);
    void __stdcall ReqEquipment(const EquipDescList& equipDescriptorList, unsigned int clientId);
    void __stdcall ReqAddItem(unsigned int& goodArchetypeId, char* hardpoint, int& count, float& status, bool& mounted, uint clientId);
}

namespace BatsBotsShipTransferFix
{
    void __stdcall GFGoodBuy(const SGFGoodBuyInfo& gbi, unsigned int clientId);
    void __stdcall ReqEquipment(const EquipDescList& equipDescriptorList, unsigned int clientId);
}

namespace Discord {
    struct ChatMessage {
        std::wstring wscCharname;
        std::wstring wscChatMessage;

    };

    struct DMMessage {
        std::string DiscordUserID;
        dpp::message DiscordMessage;

    };

    struct LastSelectClick {
        dpp::user User;
        dpp::select_click_t event;

        LastSelectClick(const dpp::user& user, const dpp::select_click_t& clickEvent)
            : User(user), event(clickEvent)
        {
        }
    };

    struct MessageListEntry {
        dpp::message Message;

    };


    struct DiscordUser {
        std::string scServerUsername;
        std::string scDiscordUsername;
        std::string scDiscordDisplayName;
        std::string scDiscordID;
    };


    //extern
    extern std::list<ChatMessage> lChatMessages;
    extern std::list <LastSelectClick> lLastSelectClick;
    extern std::map<std::string, DiscordUser> userDataMap;
    extern int iOnlinePlayers;

    //Konfig
    extern std::string scDiscordBotToken;
	extern std::string scDiscordServerID;
    extern std::string scUVChatChannelID;
    extern std::string scModRequestChannelID;
    extern std::string scModGroupID;
    extern std::string scNewsChannelID;
    extern std::string scEventChannelID;

    bool LoadSettings();
    void StartUp();
    std::string wstring_to_utf8(const std::wstring& wstr);
    std::wstring Utf8ToWString(const std::string& utf8Str);

    template<typename T>
    void GetServerstatus(const T& event);

} // namespace DiscordBot


namespace SpawnProtection
{
    extern const mstime TIMER_INTERVAL;
    void LoadSettings();
    void UpdateSpawnProtectionValidity();
    void __stdcall SystemSwitchOutComplete_AFTER(unsigned int shipId, unsigned int clientId);
    void __stdcall PlayerLaunch_AFTER(unsigned int shipId, unsigned int clientId);
    void __stdcall LaunchComplete_AFTER(unsigned int baseId, unsigned int shipId);
    void __stdcall JumpInComplete_AFTER(unsigned int systemId, unsigned int shipId);
}

namespace Storage
{
    void Initialize();
    bool UserCmd_Storage(const uint clientId, const std::wstring& arguments);
}

namespace ConnectionLimiter
{
    extern uint maxParallelConnectionsPerIpAddress;
    void __stdcall Login_After(struct SLoginInfo const& li, unsigned int clientId);
    void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2);
}

#endif