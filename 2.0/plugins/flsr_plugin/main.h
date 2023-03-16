#ifndef __MAIN_H__
#define __MAIN_H__ 1

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

//FLHOOK Includes
#include <FLHook.h>
#include <plugin.h>

//Includes
#include <list>
#include <string>
#include <windows.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <numeric>
#include <pqxx/pqxx>

//Plugin Stuff
extern PLUGIN_RETURNCODE returncode;

//Offsets
#define ADDR_CLIENT_NEWPLAYER 0x8010
#define ADDR_CRCANTICHEAT 0x6FAF0

//FilePaths
#define PLUGIN_CONFIG_FILE "\\flhook_plugins\\flsr.cfg"
#define DOCK_CONFIG_FILE "\\flhook_plugins\\flsr-dock.cfg"
#define PATHSELECTION_CONFIG_FILE "\\flhook_plugins\\flsr-pathselection.cfg"
#define CARRIER_CONFIG_FILE "\\flhook_plugins\\FLSR-Carrier.cfg"
#define CLOAK_CONFIG_FILE "\\flhook_plugins\\FLSR-Cloak.cfg"
#define FLHOOKUSER_FILE "\\flhookuser.ini"
#define SENDCASHLOG_FILE "-givecashlog.txt"
#define INSURANCE_STORE "\\flhook_plugins\\flsr-insurance\\"
#define LIBRELANCER_SDK "\\flhook_plugins\\librelancer-sdk\\"
#define CMP_DUMP_FOLDER "\\flhook_plugins\\flsr-cmpdumps\\"
#define MISSION_STORE "\\flhook_plugins\\missions\\"
#define Equip_WHITELIST_FILE "\\flhook_plugins\\FLSR-EquipWhiteList.cfg"
#define AC_REPORT_TPL "\\flhook_plugins\\flsr-cheater\\ReportTemplate.html"
#define DATADIR "..\\DATA"

//-Static
#define DISCORD_WEBHOOK_UVCHAT_FILE "C:\\Freelancer\\FLSR Public\\EXE\\flhook_plugins\\webhook.exe"
#define DISCORD_WEBHOOK_MODREQUEST_FILE "C:\\Freelancer\\FLSR Public\\EXE\\flhook_plugins\\modrequest.exe"
#define DISCORD_WEBHOOK_CHEATREPORT_FILE "C:\\Freelancer\\FLSR Public\\EXE\\flhook_plugins\\cheatreport.exe"
#define PLAYERONLINE_FILE "C:\\Caddy\\files\\Server.ini"
#define CHEATREPORT_STORE "C:\\Caddy\\files\\cheater\\"
#define DISCORD_CHAT_FILE "C:\\DiscordBot\\Chat.ini"

//SQL
#define SQLDATA "host=localhost port=5432 dbname=FLSR user=postgres password=flsr"


//AntiCheat
typedef void(__stdcall *_CRCAntiCheat)();
extern _CRCAntiCheat CRCAntiCheat_FLSR;

//Namespaces
namespace Modules {
    struct Module {
        std::string scModuleName;
        bool bModulestate;
    };

    void LoadModules();
    bool GetModuleState(std::string scModuleName);
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
    void AdminCmd_Stalk(CCmds *cmds, std::wstring Charname = L"");
    void CmdHelp_Callback(CCmds *classptr);
    bool ExecuteCommandString_Callback(CCmds *cmds, const std::wstring &wscCmd);
    void UserCMD_Clear(uint iClientID, const std::wstring &wscParam);
    void UserCMD_Contributor(uint iClientID, const std::wstring &wscParam);
    void UserCMD_DOCKACCEPT(uint iClientID, const std::wstring &wscParam);
    void UserCMD_DOCKREQUEST(uint iClientID, const std::wstring &wscParam);
    void UserCMD_ENABLECARRIER(uint iClientID, const std::wstring &wscParam);
    void UserCmd_MODREQUEST(uint iClientID, const std::wstring &wscParam);
    bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd);
    void UserCMD_SendCash$(uint iClientID, const std::wstring &wscParam);
    void UserCMD_SendCash(uint iClientID, const std::wstring &wscParam);
    void UserCmd_UV(uint iClientID, const std::wstring &wscParam);
    void UserCMD_INSURANCE_AUTOSAVE(uint iClientID, const std::wstring &wscParam);
    void UserCmd_CLOAK(uint iClientID, const std::wstring& wscParam);
    void UserCmd_UNCLOAK(uint iClientID, const std::wstring& wscParam);
    void UserCmd_HELP(uint iClientID, const std::wstring& wscParam);
    void UserCmd_Tag(uint iClientID, const std::wstring& wscParam);
    
    typedef void (*_UserCmdProc)(uint, const std::wstring &);
    struct USERCMD {
        wchar_t *wszCmd;
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
    std::vector<std::string> GetHardpointsFromCollGroup(uint iClientID);
    float CalcDisabledHardpointWorth(uint iClientID);
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
    std::vector<std::string> getHardpoints(std::string scParent, std::list<CMPDump_Entry> CMPList);

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
 
}

namespace Docking {
   
    

    struct UndockRelocate {

		bool bStalkMode;
        float fx_Undock;
        float fy_Undock;
        float fz_Undock;
        Vector pos;
        Matrix rot;
        uint iClientID;
		uint iShip;
		uint iSystem;
        
    };
 


    struct CarrierList {

        uint iCarrierID;
        uint iDockedPlayers;
        uint iBaseDocked;
        uint iSlots;
        std::string Interior;
        float fx_Undock;
        float fy_Undock;
        float fz_Undock;

        CarrierList() {
            iCarrierID = 0;
            iDockedPlayers = 0;
            iBaseDocked = 0;
            iSlots = 0;
            Interior = "";
            fx_Undock = 0.0f;
            fy_Undock = 0.0f;
            fz_Undock = 0.0f;
        }

		CarrierList(uint iCarrierID, uint iDockedPlayers, uint iBaseDocked, uint iSlots, std::string Interior, float fx_Undock, float fy_Undock, float fz_Undock) {
            this->iCarrierID = iCarrierID;
            this->iDockedPlayers = iDockedPlayers;
            this->iBaseDocked = iBaseDocked;
            this->iSlots = iSlots;
            this->Interior = Interior;
            this->fx_Undock = fx_Undock;
            this->fy_Undock = fy_Undock;
            this->fz_Undock = fz_Undock;
        }
    };

    struct CarrierDockedPlayers {

        uint iCarrierID;
        uint iPlayerID;
        float fx_Undock;
        float fy_Undock;
        float fz_Undock;

        CarrierDockedPlayers() {
            iCarrierID = 0;
            iPlayerID = 0;
            fx_Undock = 0.0f;
            fy_Undock = 0.0f;
            fz_Undock = 0.0f;
        }

		CarrierDockedPlayers(uint iCarrierID, uint iPlayerID, float fx_Undock, float fy_Undock, float fz_Undock) {
            this->iCarrierID = iCarrierID;
            this->iPlayerID = iPlayerID;
            this->fx_Undock = fx_Undock;
            this->fy_Undock = fy_Undock;
            this->fz_Undock = fz_Undock;
        }
    };

    struct CarrierDockRequest {

        uint iCarrierID;
        uint iPlayerID;
        mstime tmRequestTime;
        uint iBaseCarrierDocked;
        std::string sInterior;
        bool bSend;
        float fx_Undock;
        float fy_Undock;
        float fz_Undock;

        CarrierDockRequest() {
            iCarrierID = 0;
            iPlayerID = 0;
            tmRequestTime = 0;
            iBaseCarrierDocked = 0;
            sInterior = "test";
            bSend = false;
            fx_Undock = 0.0f;
            fy_Undock = 0.0f;
            fz_Undock = 0.0f;
        }

        CarrierDockRequest(uint iCarrierID, uint iPlayerID, mstime tmRequestTime, bool bSend, uint iBaseCarrierDocked, std::string sInterior, float fx_Undock, float fy_Undock, float fz_Undock) {
            this->iCarrierID = iCarrierID;
            this->iPlayerID = iPlayerID;
            this->tmRequestTime = tmRequestTime;
            this->iBaseCarrierDocked = iBaseCarrierDocked;
            this->sInterior = sInterior;
            this->bSend = bSend;
            this->fx_Undock = fx_Undock;
            this->fy_Undock = fy_Undock;
            this->fz_Undock = fz_Undock;
        }
    };

    struct CarrierConfig {

        uint iShipArch;
        uint iSlots;
        std::string sInterior;
        float fx_Undock;
        float fy_Undock;
        float fz_Undock;


        CarrierConfig() {
            iShipArch = 0;
            iSlots = 0;
            sInterior = "";
            fx_Undock = 0.0f;
            fy_Undock = 0.0f;
            fz_Undock = 0.0f;
        }

		CarrierConfig(uint iShipArch, uint iSlots, std::string sInterior, float fx_Undock, float fy_Undock, float fz_Undock) {
            this->iShipArch = iShipArch;
            this->iSlots = iSlots;
            this->sInterior = sInterior;
			this->fx_Undock = fx_Undock;
			this->fy_Undock = fy_Undock;
			this->fz_Undock = fz_Undock;
        }



    };

    // Carrier - Lists
    extern std::list<CarrierList> lCarrierList;
    extern std::list<CarrierDockedPlayers> lCarrierDockedPlayers;
    extern std::list<CarrierDockRequest> lCarrierDockRequest;
    extern std::list<CarrierConfig> lCarrierConfig;
    extern std::list<UndockRelocate> lUndockRelocate;


    // Carrier - Timeout
    extern mstime msRequestTimeout;

    // Carrier - Dockrange
    extern float fDockRange;


    void ClearCarrier(uint iClientID);
    void HandleUndocking(uint iClientID);
    void DockRequest3000ms();
    bool FLSR_SystemSwitchOutComplete(unsigned int iShip, unsigned int iClientID, unsigned int iSystem, bool bstalkmode = false);
    void DockOnProxyCarrierBase(std::string scBasename, uint iClientID, std::string scCarrierBase, uint iCarrierID);
    void UndockProxyBase(uint iCarrierId, uint iClientID, float fx_Undock, float fy_Undock, float fz_Undock, bool bstalkmode = false);
    void DOCKACCEPT_ALL(uint iClientID);
}

namespace Hooks {
    void __stdcall PopUpDialog(unsigned int iClientID, unsigned int buttonClicked);
    void __stdcall CharacterSelect(struct CHARACTER_ID const &cId, unsigned int iClientID);
    void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip);
    void __stdcall HkCb_AddDmgEntry(DamageList *dmg, unsigned short p1, float damage, enum DamageEntry::SubObjFate fate);
    void __stdcall ShipDestroyed(DamageList *_dmg, DWORD *ecx, uint iKill);
    void __stdcall BaseEnter_AFTER(unsigned int iBaseID, unsigned int iClientID);
    void __stdcall SPObjUpdate(struct SSPObjUpdateInfo const &ui, unsigned int iClientID);
    void __stdcall SubmitChat(CHAT_ID cId, unsigned long lP1, void const *rdlReader, CHAT_ID cIdTo, int iP2);
    int __cdecl Dock_Call(unsigned int const& iShip, unsigned int const& iDockTarget, int iCancel, enum DOCK_HOST_RESPONSE response);
	void __stdcall SPMunitionCollision(struct SSPMunitionCollisionInfo const& ci, unsigned int iClientID);
    void __stdcall JumpInComplete(unsigned int iSystemID, unsigned int iShipID);
	void __stdcall SystemSwitchOutComplete(unsigned int iShip, unsigned int iClientID);
    void __stdcall ClearClientInfo(unsigned int iClientID);
    void __stdcall FireWeapon(unsigned int iClientID, struct XFireWeaponInfo const& wpn);
    void __stdcall PlayerLaunch_After(unsigned int iShip, unsigned int iClientID);
    void __stdcall ReqAddItem(unsigned int goodID, char const* hardpoint, int count, float status, bool mounted, uint iClientID);
    void __stdcall ReqShipArch_AFTER(unsigned int iArchID, unsigned int iClientID);
	void __stdcall ReqEquipment(class EquipDescList const& edl, unsigned int iClientID);
	void __stdcall GoTradelane(unsigned int iClientID, struct XGoTradelane const& gtl);
    void __stdcall DisConnect(unsigned int iClientID, enum EFLConnection state);
    void __stdcall CreateNewCharacter_After(struct SCreateCharacterInfo const& si, unsigned int iClientID);
    void __stdcall RequestEvent(int iIsFormationRequest, unsigned int iShip, unsigned int iDockTarget, unsigned int p4, unsigned long p5, unsigned int iClientID);

    }

namespace ClientController {
    void Send_ControlMsg(bool sHook, uint iClientID, std::wstring wscText, ...);
    void Handle_ClientControlMsg(CHAT_ID cId, unsigned long lP1, void const* rdlReader, CHAT_ID cIdTo, int iP2);
}

namespace Insurance {

    extern float set_fCostPercent;
    
    extern bool Insurance_Module;
    void CreateNewInsurance(uint iClientID, bool bFreeItem);
    void UseInsurance(uint iClientID);
    void PlayerDiedEvent(bool bDied, uint iClientID);
    bool CheckPlayerDied(uint iClientID);
    bool FindHardpointCargolist(std::list<CARGO_INFO> &cargolist, CacheString &hardpoint);   
    void BookInsurance(uint iClientID, bool bFreeItem);
    std::pair<bool, bool> CheckInsuranceBooked(uint iClientID);
    std::wstring CalcInsurance(uint iClientID, bool bPlayerCMD, bool bFreeInsurance);
    bool insurace_exists(const std::string &name);
    void ReNewInsurance(uint iClientID);

    extern struct PlayerDied {

        std::wstring wscCharname;
        bool bDied;

        PlayerDied() {
            wscCharname = L"";
            bDied = false;
        }

        PlayerDied(std::wstring wscCharname, bool bDied) {
            this->wscCharname = wscCharname;
            this->bDied = bDied;
        }

    };

    extern struct BookInsuranceEvent {

        std::wstring wscCharname;
        bool bFreeItem;

        BookInsuranceEvent() {
            wscCharname = L"";
            bFreeItem = false;
        }

        BookInsuranceEvent(std::wstring wscCharname, bool bFreeItem) {
            this->wscCharname = wscCharname;
            this->bFreeItem = bFreeItem;

        }
    };

    extern struct PriceList {
        CARGO_INFO CARGO_INFO;
		GoodInfo GoodInfo;
        bool bFreeInsurance;
        bool bItemisFree;
    };

    extern struct RestoreEquip {
        CARGO_INFO CARGO_INFO;
        float fPrice;
        bool bFreeInsurance;
        bool bItemisFree;
    };

    extern std::map<std::wstring, std::list<PriceList>> mPriceList;
    extern std::list<PlayerDied> lPlayerDied;
    extern std::list<BookInsuranceEvent> lBookInsuranceEvent;
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
 
    namespace Reporting {
        void ReportCheater(uint iClientID, std::string scType, std::string sData);
        std::string CreateReport(uint iClientID, std::wstring wscType,std::wstring wscTime, std::wstring wscDETAILS);
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


namespace CustomMissions {
    struct PlayerWaypoint {
        std::string X;                              // Coord X (Client convert it to float)
        std::string Y;                              // Coord Y (Client convert it to float)
        std::string Z;                              // Coord Z (Client convert it to float)
        uint iSystemID;                             // ID of the WP-System
        uint iSolarObjectID;                        // ID of the selected Object | needed for BestPath, SystemSwitch
    };

    struct CustomMission {
		// Mission Data
        int iMissionID;                             // Mission ID
        std::string scMissionFilepath;              // Mission Filepath
        std::string scMissionName;                  // Mission Name
        std::string scMissionDesc;                  // Mission Description
        std::string scMissionType;                  // Mission Type
        bool bSPMission;                            // Single Player Mission
        bool bMPMission;                            // Multi Player Mission

		// Mission Reward
        bool bRewardCredits;                        // Reward Credits
        int iRewardCredits;                         // Reward Credits Amount
        bool bRewardReputation;                     // Reward Reputation
        int iRewardReputation;                      // Reward Reputation Amount
        std::string scRewardReputationNickname;     // Reward Reputation Nickname
        bool bRewardShip;                           // Reward Ship
        std::string scRewardShipNickname;           // Reward Ship Nickname
        bool bRewardEquip;                           // Reward Equipment
        std::string scRewardEquipNickname;           // Reward Equipment Nickname
        int iRewardEquip;                            // Reward Equipment Amount

		// Trade Mission
        std::string scTargetBaseNickname;           // Target Base Nickname
        std::string scGoodToTradeNickname;          // Good To Trade Nickname
        std::string iAmountToTrade;                 // Amount To Trade

		// Kill NPC Mission
        std::string scNPCType;                      // NPC Type
        int iAmountofNPCs;                          // Amount of NPCs
        int iAmountofWaves;                         // Amount of Waves
        bool bRewardKill;                           // Reward Kill
        bool bRewardGroup;                          // Reward Group
        bool bKillNamedNPC;                         // Kill Named NPC
        std::string scNamedNPCName;                 // Named NPC Name

		// Player Hunt Mission
        std::wstring wscPlayerCharname;             // Player Charname
        int Bounty;                                 // Bounty
        
        // Mining Mision
        std::string scGoodToMineNickname;           // Good To Mine Nickname
        int iAmountToMine;                          // Amount To Mine	

		// MissionWaypoints
        bool bSendMissionWaypoints;                 // Send MissionWaypoints to Player
        bool bSendMissionWaypointsToGroup;          // Send MissionWaypoints to Group
        std::list<PlayerWaypoint> lPlayerWaypoints; // List of Waypoints

        //POPUPs
        // MissionStart POPUP
        bool bMissionStartPopup;                    //Show a Popup on Mission start
		int iMissionStartPopupHead;                 //The ResID of the Popup Head
		int iMissionStartPopupBody;                 //The ResID of the Popup Body
        //MissionEnd POPUP
		bool bMissionEndPopup;                      //Show a Popup on Mission end
		int iMissionEndPopupHead;                   //The ResID of the Popup Head
		int iMissionEndPopupBody;                   //The ResID of the Popup Body

        //MISSIONTEXTs
        //MissonStart Text
		bool bMissionStartText;                     //Show a Text on Mission start
		int iMissionStartText;                      //The ResID of the Text
        //MissionEnd Text
		bool bMissionEndText;                       //Show a Text on Mission end
		int iMissionEndText;                        //The ResID of the Text



    };
	
    extern std::list<CustomMission> lCustomMission;

    void LoadMissions();
    void Send_WPs(uint iClientID, std::list <CustomMissions::PlayerWaypoint> lWP, bool bBestPath);

}

namespace Cloak {


    void InstallCloak(uint iClientID);
	void CloakInstallTimer2000ms();
    void WarmUpCloakTimer1000ms();
    void DoCloakingTimer250ms();
	void UncloakGroup(uint iClientID);
    void LoadCloakSettings();
    bool Check_Dock_Call(uint iShip, uint iDockTarget, uint iCancel, enum DOCK_HOST_RESPONSE response);
    bool Check_GoTradelane(unsigned int iClientID, struct XGoTradelane const& gtl);
    bool Check_Cloak(uint iClientID);
    void StartCloakPlayer(uint iClientID);
    void DoCloak(uint iClientID);
    void UncloakPlayer(uint iClientID);
    void UpdateShipEnergyTimer();
    bool Check_RequestEventFormaDocking(int iIsFormationRequest, unsigned int iShip, unsigned int iDockTarget, unsigned int p4, unsigned long p5, unsigned int iClientID);
    void CloakSync(uint iClientID);
    void KillShield(uint iClientID);

    struct CloakDeviceInfo {
		
        std::string scCloakDeviceNickname;
        uint iCloakDeviceArchID;
        float fCloakCapacity;
		float fPowerUsageToRecharge;
		float fCloakPowerUsageWhileCloaked;
        float fMinRequiredCapacityToCloak;
        bool bUseShipPowerToRecharge;
        bool bShieldDownOnCloaking;
        bool bShieldDownWhileCloaking;
		bool bCanUseCloakModule;
        int iCloakWarmUpDuration;
        int iCloakEffectDuration;
        int iUncloakEffectDuration;
		
    };

    struct PlayerCloakInfo {
        bool bInitialCloak = false;
        bool bCanCloak = false;
        bool bIsCloaking = false;
        bool bWantsCloak = false;
        bool bCloaked = false;
        int iCloakSlot = 0;
        uint iCloakDeviceArchID;
        mstime tmCloakTime = timeInMS();
		mstime tmUnCloakTime = timeInMS();
        bool bIsinGroup = false;
		uint iGroupID = 0;
        float fCloakCap = 0.0f;
		float fEnergy = 0.0f;
        CloakDeviceInfo PlayerCloakData;
		bool bShowUI = false;
        bool bAllowUncloak = true;
        bool bAllowCloak = false;
        bool bPlayerShield = false;
    };
	
    struct WarmUpCloak {
        bool bRdy = false;
        mstime msStart = 0;
    };
    
    // extern IMPORT PlayerCloakInfo PlayerCloakData[MAX_CLIENT_ID + 1];
    // extern IMPORT WarmUpCloak PlayerWarmUpCloak[MAX_CLIENT_ID + 1];

	extern std::list<CloakDeviceInfo> lCloakDeviceList;
	extern std::map<std::wstring, PlayerCloakInfo> mPlayerCloakData;
    extern std::map<std::wstring, WarmUpCloak> mPlayerWarmUpCloak;


}

namespace EquipWhiteList {

	struct EquipWhiteListEntry {
		std::string scEquipNickname;
		uint iEquipID;
		std::vector<std::pair<uint, std::string>> vShip; // ShipID, ShipNickname
	};

    extern std::list<EquipWhiteListEntry> lEquipWhiteList;

    void LoadEquipWhiteList();
    bool ReqAddItem_CheckEquipWhiteList(unsigned int goodID, char const* hardpoint, int count, float status, bool mounted, uint iClientID);
    void SendList(uint iShipArch, uint iClientID, bool oldShip);
}

namespace SQL {

    extern bool InitializeedServerData;

    void InitializePlayerDB();
    void InitializeServerData();
    void Timer2000ms();
    void Thread2000ms();
    pqxx::connection Connect();
    pqxx::result CommitQuery(std::string query);
	
}

namespace Depot {

    struct PlayerDepot {
        uint iDepotID;
        uint iBaseID;
        std::string scAccountName;
        uint iCapacity;		
    };

	struct PlayerDepotItem {
		uint iDepotID;
		uint iGoodID;
		uint iAmount;
	};

    struct PlayerCargoItem {
        uint iGoodID;
        uint iAmount;
    };

    void LoadDepotData();
    std::string GetEquipNicknameFromID(uint goodID);
    std::list<PlayerDepotItem> GetEquipFromBaseDepot(uint iClientID, bool bPrint);
    void PlayerDepotOpen(uint iClientID);
    void GetPlayerEquip(uint iClientID);

    extern std::list<PlayerDepot> lPlayerDepot;	
}


namespace PathSelection {
   
    struct Reputation{
		std::string scFactionName;
		float fReputation;
    };
    
    struct BlockedGate {
       uint iGateID;
    };

    struct UnlawfulPlayer {
        bool bisUnlawful;
        bool bisCharModified;
    };
    
	struct OpenUnlawfulMod {
		uint iClientID;
        std::string scCharname;
        std::wstring scAccountID;
        std::wstring wscAccDir;
    };

    extern std::string scStart_Base;
    extern std::string scSystem;
    extern uint iCash;
    
    extern std::list<PathSelection::OpenUnlawfulMod> lOpenUnlawfulMods;
    extern std::list<PathSelection::Reputation> lReputations;
    extern std::list<PathSelection::BlockedGate> lBlockedGates;
    extern IMPORT UnlawfulPlayer UnlawfulPlayerData[MAX_CLIENT_ID + 1];
    //extern std::map<std::wstring, UnlawfulPlayer> mUnlawfulPlayer;


    void LoadPathSelectionSettings();
    bool Check_BlockedGate(uint iShip);
    void Install_Unlawful(uint iClientID);
    void SetUnlawful(uint iClientID, std::string scCharname, std::string scState);
    void ModUnlawfulChar500ms();

}

namespace FuseControl {

    bool ReadIniFuseConfig();
    static void ReadIniFuseConfigFile(const std::string& filePath, bool bFuseIni);
    
    enum FuseType
    {
        FUSE_UNKNOWN,
        FUSE_SHIP,
        FUSE_SOLAR,
        FUSE_COLLGRP

    };
    
    struct Fuse {

        FuseType eType;
        std::string scShipNickname;
        std::string scNickname;
        float fLifeTime;
        float fHitpoint;
    };
    
    struct ClientFuse {
        Fuse oldFuse;
		Fuse newFuse;       
    };
    
    extern IMPORT ClientFuse FuseControl[MAX_CLIENT_ID + 1];
    extern std::map<uint, Fuse> mFuseMap;
}


namespace PlayerHunt {

    extern std::vector<std::string> SystemWhitelist;

	enum HuntState {
		HUNT_STATE_NONE,
        HUNT_STATE_DISCONNECTED,
        HUNT_STATE_WON,
		HUNT_STATE_HUNTING
	};

	struct ServerHuntInfo {
		std::wstring wscCharname;
		uint iTargetBase;
        uint iTargetSystem;
		HuntState eState;
		mstime tmHuntTime;
        uint iCredits;
	};

	struct LastPlayerHuntWinners {
		std::wstring wscCharname;
		uint iBaseID;
		mstime tmHuntTime;
        uint Credits;
	};
    
	struct BaseData {
		uint iBaseID;
		uint iSystemID;
        std::string scBaseNickname;

	};
    
    extern float set_fPlayerHuntMulti;
	extern float set_fPlayerHuntUpdateTick;
    extern float set_fPlayerHuntRestartDelay;
	extern int set_iPlayerHuntMinSystems;
    
	extern std::list <LastPlayerHuntWinners> lLastPlayerHuntWinners;
	extern ServerHuntInfo ServerHuntData;
    
    uint getRandomSysteminRange(uint iClientID);
    BaseData getRandomBaseInSystem(uint iSystemID, uint iClientID);
    BaseData getTargetBase(uint iClientID);

}

#endif