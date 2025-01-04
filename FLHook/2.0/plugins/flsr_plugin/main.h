#ifndef __MAIN_H__
#define __MAIN_H__ 1

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

//Disable Warnings
#pragma warning( disable : 4251 ) // needs to have dll-interface to be used by clients of class

//DISCORD Includes
#include <dpp/dpp.h>

//SQL Includes
#include <SQLiteCpp/SQLiteCpp.h>

//FLHOOK Includes
#include <FLHook.h>
#include <plugin.h>

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

//Plugin Stuff
extern PLUGIN_RETURNCODE returncode;

//Offsets
#define ADDR_CLIENT_NEWPLAYER 0x8010
#define ADDR_CRCANTICHEAT 0x6FAF0

// Mutex-Objekt deklarieren
extern std::mutex m_Mutex;

//AntiCheat
typedef void(__stdcall *_CRCAntiCheat)();
extern _CRCAntiCheat CRCAntiCheat_FLSR;

//Namespaces

namespace Globals {

    //FilePaths
    const std::string PLUGIN_CONFIG_FILE = "\\flhook_plugins\\flsr.cfg";
    const std::string CARRIER_CONFIG_FILE = "\\flhook_plugins\\FLSR-Carrier.cfg";
    const std::string CLOAK_CONFIG_FILE = "\\flhook_plugins\\FLSR-Cloak.cfg";
    const std::string CRAFTING_CONFIG_FILE = "\\flhook_plugins\\FLSR-Crafting.cfg";
    const std::string LOOTBOXES_CONFIG_FILE = "\\flhook_plugins\\FLSR-LootBoxes.cfg";
    const std::string FLHOOKUSER_FILE = "\\flhookuser.ini";
    const std::string SENDCASHLOG_FILE = "-givecashlog.txt";
    const std::string INSURANCE_STORE = "\\flhook_plugins\\flsr-insurance\\";
    const std::string LIBRELANCER_SDK = "\\flhook_plugins\\librelancer-sdk\\";
    const std::string CMP_DUMP_FOLDER = "\\flhook_plugins\\flsr-cmpdumps\\";
    const std::string Equip_WHITELIST_FILE = "\\flhook_plugins\\FLSR-EquipWhiteList.cfg";
    const std::string AC_REPORT_TPL = "\\flhook_plugins\\flsr-cheater\\ReportTemplate.html";
    const std::string DATADIR = "..\\DATA";

    //SQL 
    enum SQLOpenFlags {
        OPEN_READONLY = 0x00000001,
        OPEN_READWRITE = 0x00000002,
        OPEN_CREATE = 0x00000004,
        OPEN_DELETEONCLOSE = 0x00000008,
        OPEN_EXCLUSIVE = 0x00000010,
        OPEN_AUTOPROXY = 0x00000020,
        OPEN_URI = 0x00000040,
        OPEN_MEMORY = 0x00000080,
        OPEN_MAIN_DB = 0x00000100,
        OPEN_TEMP_DB = 0x00000200,
        OPEN_TRANSIENT_DB = 0x00000400,
        OPEN_MAIN_JOURNAL = 0x00000800,
        OPEN_TEMP_JOURNAL = 0x00001000,
        OPEN_SUBJOURNAL = 0x00002000,
        OPEN_SUPER_JOURNAL = 0x00004000,
        OPEN_NOMUTEX = 0x00008000,
        OPEN_FULLMUTEX = 0x00010000,
        OPEN_SHAREDCACHE = 0x00020000,
        OPEN_PRIVATECACHE = 0x00040000,
        OPEN_WAL = 0x00080000,
        OPEN_NOFOLLOW = 0x01000000,
        OPEN_EXRESCODE = 0x02000000
    };


}

namespace Modules {
    struct Module {
        std::string scModuleName;
        bool bModulestate;
    };

    void LoadModules();
    bool GetModuleState(std::string scModuleName);
    bool SetModuleState(const std::string& scModuleName, bool bModuleState);
    void SwitchModuleState(std::string scModuleName);
	
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
    bool ExecuteCommandString_Callback(CCmds *cmds, const std::wstring &wscCmd);
    void UserCMD_Contributor(uint iClientID, const std::wstring &wscParam);
    void UserCmd_MODREQUEST(uint iClientID, const std::wstring &wscParam);
    bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd);
    void UserCMD_SendCash$(uint iClientID, const std::wstring &wscParam);
    void UserCMD_SendCash(uint iClientID, const std::wstring &wscParam);
    void UserCmd_UV(uint iClientID, const std::wstring &wscParam);
    void UserCmd_HELP(uint iClientID, const std::wstring& wscParam);
    void UserCmd_PLAYERHUNT(uint iClientID, const std::wstring& wscParam);
    
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
    
    struct ParentMap {
		std::string scParent;
		std::string scFirstParent;
	};
   




    extern std::list <CMPDump_Exception> lCMPUpdateExceptions;
    void HkNewPlayerMessage(uint iClientID, struct CHARACTER_ID const &cId);
    void HkClearMissionBug(int clientID);
    extern std::wstring CS_wscCharBefore;
    extern std::string base64_chars;
    extern std::map<uint, HashMap> mNicknameHashMap;
    static inline bool is_base64(unsigned char c);
    std::string base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len);
    std::string base64_decode(std::string const &encoded_string);
    std::vector<std::string> HkGetCollisionGroups(uint iClientID, bool bOnly);
    bool startsWith(std::string_view str, std::string_view prefix);
    bool endsWith(std::string_view str, std::string_view suffix);
    void replace_first(std::string &s, std::string const &toReplace,std::string const &replaceWith);
    std::string StringBetween(std::string str, std::string first, std::string last);
    bool GetB(std::string svalue);
    static void ReadIniNicknameFile(const std::string& filePath);
    bool ReadIniNicknames();
    Matrix Rz(float angleDeg);
    HK_ERROR FLSR_HkFLIniGet(const std::wstring& wscCharname, const std::wstring& wscKey, std::wstring& wscRet);
    HK_ERROR FLSR_HkFLIniWrite(const std::wstring & wscCharname, const std::wstring & wscKey, const std::wstring & wscValue);
    bool IsPlayerInRange(uint iClientID, uint iClientID2, float fDistance);
    void get_cmpfiles(const std::filesystem::path& path);
    void get_cmpExceptions();

    bool isValidPlayer(uint iClientID, bool bCharfile);
    void CharSelectMenu();

    //Reputation Stuff
    //Reputation callback struct
    struct RepCB
    {
        uint iGroup;
        uint iNameLen;
        char szName[16];
    };
    typedef bool(__stdcall* _RepCallback)(RepCB* rep);

    extern std::list<RepCB> lstTagFactions;

    bool __stdcall RepCallback(RepCB* rep);
    std::list<RepCB> HkGetFactions();
    bool __stdcall RepEnumCallback(RepCB* rep);
    void HkEnumFactions(_RepCallback callback);
    uint GetiGroupOfFaction(std::wstring wscParam);


    //Shortest Path

    // Definition for a graph node
    struct Node {
        std::string system;
        int distance;
        std::string previous;

        bool operator>(const Node& other) const {
            return distance > other.distance;
        }
    };

    // Definition for a graph edge
    struct Edge {
        std::string start;
        std::string end;
        int distance;
    };
    
    std::vector<std::string> FindShortestPath(const std::vector<Edge>& edges, const std::string& start, const std::string& end);
    void ParsePathsFromFile(std::vector<Edge>& edges);
    std::vector<std::string> GetShortestPath(std::string start, std::string end);
    int CountShortestPath(std::string start, std::string end);

    HK_ERROR FLSRHkAddEquip(const std::wstring& wscCharname, uint iGoodID,const std::string& scHardpoint, bool bMounted);
    HK_ERROR FLSRHkAddCargo(const std::wstring& wscCharname, uint iGoodID, int iCount, bool bMission);
    float GetAveragePingOfAllPlayers();
    std::string sha1(const std::string& input);
    std::string typeToString(Archetype::AClassType aType);
 
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
    void __stdcall BaseEnter_AFTER(unsigned int iBaseID, unsigned int iClientID);
    void __stdcall SPObjUpdate(struct SSPObjUpdateInfo const &ui, unsigned int iClientID);
    int __cdecl Dock_Call(unsigned int const& iShip, unsigned int const& iDockTarget, int iCancel, enum DOCK_HOST_RESPONSE response);
	void __stdcall SPMunitionCollision(struct SSPMunitionCollisionInfo const& ci, unsigned int iClientID);
    void __stdcall JumpInComplete(unsigned int iSystemID, unsigned int iShipID);
	void __stdcall SystemSwitchOutComplete(unsigned int iShip, unsigned int iClientID);
    void ClearClientInfo(unsigned int iClientID);
    void __stdcall FireWeapon(unsigned int iClientID, struct XFireWeaponInfo const& wpn);
    void __stdcall PlayerLaunch_After(unsigned int iShip, unsigned int iClientID);
    void __stdcall DisConnect(unsigned int iClientID, enum EFLConnection state);
    void SendDeathMsg(const std::wstring& wscMsg, uint iSystemID, uint iClientIDVictim, uint iClientIDKiller);
    }

namespace Insurance
{
    extern float insuranceEquipmentCostFactor;

    void CreateNewInsurance(const uint clientId, bool onlyFreeItems);
    void UseInsurance(const uint clientId);
    bool IsInsuranceRequested(const uint clientId);
    bool IsInsurancePresent(const uint clientId);
    void UserCMD_INSURANCE(const uint clientId, const std::wstring& argumentsWS);
    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId);
    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId);
    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete);
}

namespace AntiCheat {

    namespace TimingAC {    
        void Init(unsigned int iClientID);
        void CheckTimeStamp(struct SSPObjUpdateInfo const &pObjInfo, unsigned int iClientID);       
    }
        
    namespace SpeedAC {
		void Init(uint iClientID);
        void UpdateShipSpeed(uint iClientID);
        float GetPlayerAllowedSpeed(uint iClientID, enum ENGINE_STATE state);
        bool CheckClientSpeed(uint iClientID, std::vector<float>& vecTimes, std::vector<float>& vecDistances, enum ENGINE_STATE engineState);
        void vDunno1(uint iClientID, mstime delay);
        void vDunno2(uint iClientID);
        int iDunno3(unsigned int const& iShip, unsigned int const& iDockTarget, int iCancel, enum DOCK_HOST_RESPONSE response);
        void CheckSpeedCheat(struct SSPObjUpdateInfo const& pObjInfo, unsigned int iClientID);
    }

    namespace PowerAC {
        void Init(unsigned int iClientID);
        void Setup(unsigned int iClientID);
        void FireWeapon(unsigned int iClientID, struct XFireWeaponInfo const wpn);
    }
    
    namespace DataGrab {
        void CharnameToFLHOOKUSER_FILE(uint iClientID);
    }

    struct AC_INFO {
        //Timing Detecion
        float dClientTimestamp;
        float dServerTimestamp;
        int iRateDetectCountTimingSpeed;
        bool bTimingSpeedCheater = false;
        //Speed Detection
        float fAllowedCruiseSpeed;
        float fAllowedThrusterSpeed; 
        float fAllowedEngineSpeed;
        float fAllowedTradelaneSpeed;
        float fAllowedStrafeSpeed;
        int iRateDetectSpeed;
        std::map<enum ENGINE_STATE, std::vector<float>> vecDistances;
        std::map<enum ENGINE_STATE, std::vector<float>> vecTimes;
        int iSpeedDetections;
        enum ENGINE_STATE engineState;
        mstime tmSpeedExceptionTimeout;
        double fLastSpeedTimestamp;
        Vector vLastPos;
        mstime tmCheckTime;
		bool bSpeedCheater = false;
		//Power Detection
        bool bSetupPowerCheatDet;
        float fMaxCapacity;
        float fChargeRate;
        float fCurEstCapacity;
        mstime tmLastPowerUpdate;
        float fMinCapacity;
        bool bPowerCheater = false;

    };

    extern IMPORT AC_INFO AC_Info[MAX_CLIENT_ID + 1];
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

namespace SolarInvincibility
{
    void LoadSettings();
    void Initialize();
}

namespace EquipWhiteList
{
    void LoadEquipWhiteList();
    void ProcessChangedEquipments();
    extern const uint TIMER_INTERVAL;
    void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
    void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
    void __stdcall ReqEquipment_AFTER(class EquipDescList const& equipDescriptorList, unsigned int clientId);
    void __stdcall ReqAddItem_AFTER(unsigned int goodArchetypeId, char const* hardpoint, int count, float status, bool mounted, uint clientId);
    void __stdcall ReqShipArch_AFTER(unsigned int archetypeId, unsigned int clientId);
}

namespace SQL {
    

    extern std::string scDbName;
    void InitializeDB();
}

namespace PlayerHunt {

    extern std::vector<std::string> SystemWhitelist;

	enum HuntState {
		HUNT_STATE_NONE,
        HUNT_STATE_LOST,
		HUNT_STATE_HUNTING
	};

	struct ServerHuntInfo {
		std::wstring wscCharname;
		uint iTargetBase;
        uint iTargetSystem;
		std::wstring wscTargetSystem;
		std::wstring wscTargetBase;
		HuntState eState;
        uint iReward;
        uint iHuntCredits;
        std::list<std::string> lSystems;
	};

	struct LastPlayerHuntWinners {
		std::wstring wscCharname;
		uint iBaseID;
        uint Credits;
	};
    
	struct BaseData {
		uint iBaseID;
		uint iSystemID;
        std::string scBaseNickname;

	};
    
    extern float set_fRewardMultiplicator;
	extern int set_iMinTargetSystemDistance;
    extern int set_iMinCredits;
    extern int set_iMaxCredits;
    extern int set_iMinPlayer;
    
	extern std::list <LastPlayerHuntWinners> lLastPlayerHuntWinners;
	extern ServerHuntInfo ServerHuntData;
    
    uint getRandomSysteminRange(uint iClientID);
    BaseData getRandomBaseInSystem(uint iSystemID, uint iClientID);
    BaseData getTargetBase(uint iClientID);
    void Start_PlayerHunt(uint iClientID, const std::wstring& wscParam);
    void PlayerHuntMulti(uint iClientID, uint iPlayerSystemID);
    void CalcReward();
    void CheckSystemReached(uint iClientID, uint iPlayerSystemID);
    void CheckDock(uint iBaseID, uint iClientID);
    void CheckDisConnect(uint iClientID);
    void CheckDied(uint iClientID, uint iClientKillerID, Tools::eDeathTypes DeathType);
    void LoadPlayerHuntSettings();
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
        std::string Nickname;
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
    extern std::list<ChatMessage> lModMessages;
    extern std::list<DMMessage> lDMMessages;
    extern std::list <LastSelectClick> lLastSelectClick;
    extern std::list<MessageListEntry> lNewsList;
    extern std::list<MessageListEntry> lEventList;
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


    extern int iRenameCost;

    bool LoadSettings();
    void StartUp();
    void CommandUptime(const dpp::slashcommand_t& event);
    std::string wstring_to_utf8(const std::wstring& wstr);
    std::wstring Utf8ToWString(const std::string& utf8Str);
    std::string GeneratePassword();

    //Pages
    void CharManagerPageMenu(dpp::cluster& DiscordBot, const dpp::button_click_t& event);

    //Embeds
    template<typename T>
    void BankEmbed(const T& event);

    //Modals
    template<typename T>
    void LinkModal(const T& event);
    template<typename T>
    void CharRenameModal(const T& event);
    template<typename T>
    void BankTransferModal(const T& event, const std::string modal_id);
    template<typename T>
    void GetServerstatus(const T& event);


    //Helper
    std::string GetFormComponentValue(const dpp::form_submit_t& event, const std::string& customId);

    //Charmanager Stuff
    std::string CharManager_Insert(const std::string& charfile, const std::string& discordID, const std::string& password);
    void CharManager_DeleteInvalidEntries();
    std::string GetValidationForChar(const std::string& charfile);
    void UpdateValidationForChar(const std::string& charfile);
    std::string GetDiscordIDForChar(const std::string& charfile);
    bool IsCharacterLinkedWithDiscordID(const std::string& discordID, const std::string& charfile);
    bool CharManager_Rename(const std::string& oldcharfile, const std::string& newcharfile);
    bool CharManager_UpdateCharname(const std::string& charfile, const std::string& charname);
    std::string GetCreditsForDiscordAccount(const std::string& discordAccount);
    bool UpdateCreditsForDiscordAccount(const std::string& discordAccount, const std::string& credits, bool bAdd);
    bool DoesDiscordAccountHaveValidChars(const std::string& discordAccount);
    std::string GetUserIDByDiscordName(const std::string& discordName);
    void Update_NewsList(dpp::cluster &DiscordBot);
    void Update_EventList(dpp::cluster &DiscordBot);

    std::string GetDiscordUsername(const dpp::user& dppUser);
    bool containsWhitespace(const std::string& str);

} // namespace DiscordBot


namespace SpawnProtection
{
    extern const mstime TIMER_INTERVAL;
    bool LoadSettings();
    void UpdateInvincibleStates();
    void __stdcall SystemSwitchOutComplete_AFTER(unsigned int shipId, unsigned int clientId);
    void __stdcall PlayerLaunch_AFTER(unsigned int shipId, unsigned int clientId);
    void __stdcall LaunchComplete_AFTER(unsigned int baseId, unsigned int shipId);
    void __stdcall JumpInComplete_AFTER(unsigned int systemId, unsigned int shipId);
}

namespace GroupReputation
{
    void InitializeWithGameData();
    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId);
}

namespace Storage
{
    void InitializeStorageSystem();
    bool UserCmd_Storage(const uint clientId, const std::wstring& arguments);
}

//namespace SrvCtrlObj {
namespace SrvCtrlObj {

    struct SrvObj {
        //Name
        std::string scSrvObjNickname;

        //Type
        std::string scType;

        //Coords
        float x;
        float y;
        float z;

        //Orientation
        Matrix m;

        //System
        std::string scSystem;

        //ShipArch
        std::string scShipArch;

        //Loadout
        std::string scLoadOut;

        //AI
        pub::AI::SetPersonalityParams p;
    };

   // extern std::list<SrvObj> lstSrvObjs;


}// namespace SrvCtrlObj

#endif