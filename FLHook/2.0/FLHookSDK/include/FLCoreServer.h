//////////////////////////////////////////////////////////////////////
//	Project FLCoreSDK v1.1, modified for use in FLHook Plugin version
//--------------------------
//
//	File:			FLCoreServer.h
//	Module:			FLCoreServer.lib
//	Description:	Interface to Server.dll
//
//	Web: www.skif.be/flcoresdk.php
//  
//
//////////////////////////////////////////////////////////////////////
#ifndef _FLCORESERVER_H_
#define _FLCORESERVER_H_

#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "flmap.h"

#include "FLCoreDefs.h"
#include "FLCoreCommon.h"

#pragma comment( lib, "FLCoreServer.lib" )

#define POPUPDIALOG_BUTTONS_LEFT_YES 1
#define POPUPDIALOG_BUTTONS_CENTER_NO 2
#define POPUPDIALOG_BUTTONS_RIGHT_LATER 4
#define POPUPDIALOG_BUTTONS_CENTER_OK 8


struct CHAT_ID
{
	uint iID;
};

enum DOCK_HOST_RESPONSE
{
	ACCESS_DENIED = 1,
	DOCK_DENIED = 2,
	DOCK_IN_USE = 3,
	PROCEED_DOCK = 4,
	DOCK = 5,
};

enum MissionMessageType
{
	MissionMessageType_Failure, // mission failure, offers Respawn buttons
	MissionMessageType_Type1, // objective
	MissionMessageType_Type2, // objective
	MissionMessageType_Success, // mission success
};

struct SSPUseItem
{
	uint iUserShip;
	ushort sItemId;
	ushort sAmountUsed;
};

struct SSPMunitionCollisionInfo
{
	uint projectileArchID;  //0
	uint projectileOwner; //4
	uint targetObjId; //8
	ushort subObjId; //12
	Vector impactPosition; // 16
};

enum DestroyType
{
	VANISH = 0,
	EXPLODE = 1
};

struct SSPObjCollisionInfo
{
	DWORD dwTargetShip;
};

struct XActivateObject
{
	uint	spaceObjId;
	bool	activate;
	uint	dockIndex;
};

struct XActivateEquip
{
	uint	iSpaceID;
	ushort	sID;
	bool	bActivate;
};

struct XActivateCruise
{
	uint	iShip;
	bool	bActivate;
};

struct XActivateThrusters
{
	uint	iShip;
	bool	bActivate;
};

struct SGFGoodSellInfo
{
	long	l1;
	uint	iArchID;
	int		iCount;
};

struct SGFGoodBuyInfo
{
	uint iBaseID;
	ulong lNull;
	uint iGoodID;
	int iCount;
};

struct XFireWeaponInfo
{
	uint object;
	Vector target;
	st6::vector<ushort> hpIds;
};

struct XSetManeuver
{
	uint iShipFrom;
	uint IShipTo;
	uint iFlag;
};

struct XSetTarget
{
	uint iShip;
	ushort iSlot;
	uint iSpaceID;
	ushort iSubObjID;
};

struct SSPObjUpdateInfoSimple
{
	uint iShip;
	Quaternion vDir;
	Vector vPos;
	float fTimestamp;
	float throttle;
	char state;
};

struct SSPObjUpdateInfo
{
	uint iShip;
	Quaternion vDir;
	Vector vPos;
	double fTimestamp;
	float throttle;
	char cState;
};

struct XJettisonCargo
{
	uint iShip;
	ushort iSlot;
	uint iCount;
};

struct XTractorObjects
{
	uint shipId;
	uint dunno;
	st6::vector<uint> tractoredLootIds;
};

struct XGoTradelane
{
	uint shipId;
	uint enteredTradeLaneSpaceObjId;
	uint nextTradeLaneSpaceObjId;
};

struct CAccountListNode
{
	CAccountListNode* next;
	CAccountListNode* prev;
	uint iDunno1;
	wchar_t* wszCharname;
	uint iDunno2[32];
};

enum ConnectionType
{
	JUMPHOLE
};

struct XRequestBestPathEntry
{
	Vector position;
	uint objId; // Ignored for Server.RequestBestPath
	uint systemId;
};

struct XRequestBestPath
{
	int repId; // PlayerData.iReputation
	int waypointCount; // Ignored for Server.RequestBestPath
	bool noPathFound; // Ignored for Server.RequestBestPath
	XRequestBestPathEntry entries[2]; // Server.RequestBestPath requires exactly 2, otherwise variable length
};

enum class MissionListEmptyReason : uint
{
	NotEmpty = 0,
	BadReputation = 1,
	TooLowLevel = 2,
	TooManyPlayers = 3
};

class IMPORT CAccount
{
public:
	CAccount(class CAccount const&);
	CAccount(void);
	virtual ~CAccount(void);
	class CAccount& operator=(class CAccount const&);
	void AppendCharacterNames(st6::list<st6::wstring&>&);
	void DeleteCharacterFromID(st6::string&);
	void ForceLogout(void);
	void InitFromFolder(char const*);

public:
	uint vftable;
	wchar_t* wszAccID;
	uint iDunno2[7];
	CAccountListNode* pFirstListNode;
	uint iNumberOfCharacters;
	uint iDunno4[32];
};

namespace BaseGroupMessage
{
	enum Type;
};

struct IObjRW;

struct MetaListNode
{
	MetaListNode* next;
	MetaListNode* prev;
	IObjRW* value;
};

struct MetaList
{
	uint vtable;
	MetaListNode* start;
	MetaListNode* end;
	uint dunno[2];
};

struct Observer
{
	uint vtable;
	uint dunno[11];
	double timestamp;
	uint clientId;
	Vector position;
	uint dunno2[30]; //unknown size
};

struct IMPORT StarSystem
{
	unsigned int count_players(unsigned int)const;

public:
	uint vftable; //0
	uint dunno0; //4
	st6::list<Observer> observerList; //8
	uint dunno1[8]; //20, first 3 elements are another st6list
	MetaList shipList; // 52/13
	MetaList lootList; // 72/18
	MetaList solarList; // 92/23
	MetaList guidedList; // 112
	MetaList bulletList; // 132
	MetaList mineList; // 152
	MetaList counterMeasureList; // 172
	MetaList asteroidList; // 192
};

struct StarSystemMock
{
	uint systemId;
	StarSystem starSystem;
};

namespace SysDB
{
	IMPORT st6::map<uint, StarSystem, st6::less<uint>, st6::allocator<StarSystem>> SysMap;
};

class IMPORT CPlayerGroup
{
public:
	CPlayerGroup(class CPlayerGroup const&);
	CPlayerGroup(void);
	virtual ~CPlayerGroup(void);
	class CPlayerGroup& operator=(class CPlayerGroup const&);
	bool AddInvite(unsigned int);
	bool AddMember(unsigned int);
	bool DelInvite(unsigned int);
	bool DelMember(unsigned int);
	void DeliverChat(unsigned long, void const*);
	static class CPlayerGroup* FromGroupID(unsigned int);
	unsigned int GetID(void);
	unsigned int GetInviteCount(void);
	unsigned int GetMember(int)const;
	unsigned int GetMemberCount(void);
	unsigned int GetMissionID(void);
	unsigned int GetMissionSetBy(void);
	void HandleClientLogout(unsigned int);
	bool IsFull(void);
	bool IsInvited(unsigned int);
	bool IsMember(unsigned int);
	void RewardMembers(int);
	void SendChat(int, unsigned short const*, ...);
	void SendGroup(enum BaseGroupMessage::Type, unsigned int);
	void SetMissionID(unsigned int, unsigned int);
	void SetMissionMessage(struct CSetMissionMessage&);
	void SetMissionObjectives(struct CMissionObjectives&);
	void StoreMemberList(st6::vector<unsigned int>&);

	static class st6::map<unsigned int const, class CPlayerGroup*, struct st6::less<unsigned int const>, class st6::allocator<class CPlayerGroup*>>  s_GroupIDToGroupPtrMap;
	static unsigned int  s_uiGroupID;

public:
	unsigned char data[OBJECT_DATA_SIZE];
};

struct IMPORT IServerImpl
{
	IServerImpl(struct IServerImpl const&);
	IServerImpl(void);
	struct IServerImpl& operator=(struct IServerImpl const&);
	virtual void AbortMission(unsigned int, unsigned int);
	virtual void AcceptTrade(unsigned int, bool);
	virtual void ActivateCruise(unsigned int, struct XActivateCruise const&);
	virtual void ActivateEquip(unsigned int, struct XActivateEquip const&);
	virtual void ActivateThrusters(unsigned int, struct XActivateThrusters const&);
	virtual void AddTradeEquip(unsigned int, struct EquipDesc const&);
	virtual void BaseEnter(unsigned int, unsigned int);
	virtual void BaseExit(unsigned int, unsigned int);
	virtual void BaseInfoRequest(unsigned int, unsigned int, bool);
	virtual void CharacterInfoReq(unsigned int, bool);
	virtual void CharacterSelect(struct CHARACTER_ID const&, unsigned int);
	virtual void CharacterSkipAutosave(unsigned int);
	virtual void CommComplete(unsigned int, unsigned int, unsigned int, enum CommResult);
	virtual void Connect(char const*, unsigned short*);
	virtual void CreateNewCharacter(struct SCreateCharacterInfo const&, unsigned int);
	virtual void DelTradeEquip(unsigned int, struct EquipDesc const&);
	virtual void DestroyCharacter(struct CHARACTER_ID const&, unsigned int);
	virtual void DisConnect(unsigned int, enum EFLConnection);
	virtual void Dock(unsigned int const&, unsigned int const&);
	virtual void DumpPacketStats(char const*);
	virtual void ElapseTime(float);
	virtual void FireWeapon(unsigned int, struct XFireWeaponInfo const&);
	virtual void GFGoodBuy(struct SGFGoodBuyInfo const&, unsigned int);
	virtual void GFGoodSell(struct SGFGoodSellInfo const&, unsigned int);
	virtual void GFGoodVaporized(struct SGFGoodVaporizedInfo const&, unsigned int);
	virtual void GFObjSelect(unsigned int, unsigned int);
	virtual unsigned int GetServerID(void);
	virtual char const* GetServerSig(void);
	void GetServerStats(struct ServerStats&);
	virtual void GoTradelane(unsigned int, struct XGoTradelane const&);
	virtual void Hail(unsigned int, unsigned int, unsigned int);
	virtual void InitiateTrade(unsigned int, unsigned int);
	virtual void InterfaceItemUsed(unsigned int, unsigned int);
	virtual void JettisonCargo(unsigned int, struct XJettisonCargo const&);
	virtual void JumpInComplete(unsigned int, unsigned int);
	virtual void LaunchComplete(unsigned int, unsigned int);
	virtual void LocationEnter(unsigned int, unsigned int);
	virtual void LocationExit(unsigned int, unsigned int);
	virtual void LocationInfoRequest(unsigned int, unsigned int, bool);
	virtual void Login(struct SLoginInfo const&, unsigned int);
	virtual void MineAsteroid(unsigned int, class Vector const&, unsigned int, unsigned int, unsigned int, unsigned int);
	virtual void MissionResponse(unsigned int, unsigned long, bool, unsigned int);
	virtual void MissionSaveB(unsigned int, unsigned long);
	virtual void NewCharacterInfoReq(unsigned int);
	virtual void OnConnect(unsigned int);
	virtual void PlayerLaunch(unsigned int, unsigned int);
	virtual void PopUpDialog(unsigned int, unsigned int);
	virtual void PushToServer(class CDAPacket*);
	virtual void RTCDone(unsigned int, unsigned int);
	virtual void ReqAddItem(unsigned int goodArchetypeId, char const* hardpoint, int count, float status, bool mounted, unsigned int clientId);
	virtual void ReqCargo(class EquipDescList const&, unsigned int);
	virtual void ReqChangeCash(int, unsigned int);
	virtual void ReqCollisionGroups(class st6::list<struct CollisionGroupDesc, class st6::allocator<struct CollisionGroupDesc> > const&, unsigned int);
	virtual void ReqDifficultyScale(float, unsigned int);
	virtual void ReqEquipment(class EquipDescList const&, unsigned int);
	virtual void ReqHullStatus(float, unsigned int);
	virtual void ReqModifyItem(unsigned short, char const*, int, float, bool, unsigned int);
	virtual void ReqRemoveItem(unsigned short, int, unsigned int);
	virtual void ReqSetCash(int, unsigned int);
	virtual void ReqShipArch(unsigned int, unsigned int);
	virtual void RequestBestPath(unsigned int, unsigned char*, int);
	virtual void RequestCancel(int, unsigned int, unsigned int, unsigned long, unsigned int);
	virtual void RequestCreateShip(unsigned int);
	virtual void RequestEvent(int, unsigned int, unsigned int, unsigned int, unsigned long, unsigned int);
	virtual void RequestGroupPositions(unsigned int, unsigned char*, int);
	virtual void RequestPlayerStats(unsigned int, unsigned char*, int);
	virtual void RequestRankLevel(unsigned int, unsigned char*, int);
	virtual void RequestTrade(unsigned int, unsigned int);
	virtual void SPBadLandsObjCollision(struct SSPBadLandsObjCollisionInfo const&, unsigned int);
	virtual void SPMunitionCollision(struct SSPMunitionCollisionInfo const&, unsigned int);
	virtual void SPObjCollision(struct SSPObjCollisionInfo const&, unsigned int);
	virtual void SPObjUpdate(struct SSPObjUpdateInfo const&, unsigned int);
	virtual void SPRequestInvincibility(unsigned int, bool, enum InvincibilityReason, unsigned int);
	virtual void SPRequestUseItem(struct SSPUseItem const&, unsigned int);
	virtual void SPScanCargo(unsigned int const&, unsigned int const&, unsigned int);
	virtual void SaveGame(struct CHARACTER_ID const&, unsigned short const*, unsigned int);
	virtual void SetActiveConnection(enum EFLConnection);
	virtual void SetInterfaceState(unsigned int, unsigned char*, int);
	virtual void SetManeuver(unsigned int, struct XSetManeuver const&);
	virtual void SetMissionLog(unsigned int, unsigned char*, int);
	virtual void SetTarget(unsigned int, struct XSetTarget const&);
	virtual void SetTradeMoney(unsigned int, unsigned long);
	virtual void SetVisitedState(unsigned int, unsigned char*, int);
	virtual void SetWeaponGroup(unsigned int, unsigned char*, int);
	virtual void Shutdown(void);
	virtual bool Startup(struct SStartupInfo const&);
	virtual void StopTradeRequest(unsigned int);
	virtual void StopTradelane(unsigned int, unsigned int, unsigned int, unsigned int);
	virtual void SubmitChat(struct CHAT_ID, unsigned long, void const*, struct CHAT_ID, int);
	virtual bool SwapConnections(enum EFLConnection, enum EFLConnection);
	virtual void SystemSwitchOutComplete(unsigned int, unsigned int);
	virtual void TerminateTrade(unsigned int, int);
	virtual void TractorObjects(unsigned int, struct XTractorObjects const&);
	virtual void TradeResponse(unsigned char const*, int, unsigned int);
	virtual int Update(void);

public:
	unsigned char data[OBJECT_DATA_SIZE];
};

struct CollisionGroupDescList
{
	st6::list<CollisionGroupDesc> data;
};

struct Rumor
{
	uint IDS;
	uint rumorLevel;
};

struct VNpc
{
	enum class NpcMissionStatus
	{
		NotOnAMissionForThisNpc,
		OnAMissionForThisNpc,
		CompletedMissionForThisNpc
	};

	uint baseHash;
	uint npcHash;
	int interactionCount;
	NpcMissionStatus missionStatus = NpcMissionStatus::NotOnAMissionForThisNpc;
};

struct TLException
{
	uint startRing;
	uint nextRing;
};
struct MPlayerDataSaveStruct
{
	uint unkPtr1;                        // 0
	uint clientId1;                      // 4
	uint unkInt1;                        // 8
	uint clientId2;                      // 12
	uint padding4;                       // 16
	uint padding5;                       // 20
	uint unkPtr2;                        // 24
	uint unkPtr3;                        // 28
	bool padding9;                       // 32
	bool canDock;                        // 33
	uint canDock2;                       // 36
	st6::list<uint> dockExceptions;      // 40
	bool canTL;                          // 52
	uint padding51;                      // 56
	st6::list<TLException> tlExceptions; // 60
	FlMap<uint, uint> killedShips;       // 72
	FlMap<uint, uint> rmCompleted;       // 92
	FlMap<uint, uint> rmAborted;         // 112
	FlMap<uint, uint> rmFailed;          // 132
	float totalCashEarned;               // 156
	float totalTimePlayed;               // 160
	st6::vector<uint> visitedSystems;    // 164
	st6::vector<uint> visitedBases;      // 180
	st6::vector<uint> visitedHoles;      // 196
	uint padding52;                      // 208
	uint padding53;                      // 212
	uint padding54;                      // 216
	uint padding55;                      // 220 // recalculates rank when equal 42?
	uint padding56;                      // 224
	uint padding57;                      // 228
	uint padding58;                      // 232
	st6::vector<VNpc> visitedNPCs;       // 236
	st6::vector<Rumor> receivedRumors;   // 252
};

struct CHARACTER_ID
{
	CHARACTER_ID(void);
	CHARACTER_ID const& operator=(CHARACTER_ID const&);
	bool operator<(CHARACTER_ID const& a) const { return _stricmp(charFilename, a.charFilename) < 0; }
	bool operator==(CHARACTER_ID const& a) const { return _stricmp(charFilename, a.charFilename) == 0; }
	bool operator!=(CHARACTER_ID const& a) const { return _stricmp(charFilename, a.charFilename) != 0; }
	void invalidate(void);
	bool is_valid(void)const;

public:
	char charFilename[512];
};

struct CharacterData
{
	CHARACTER_ID characterCode;
	st6::wstring name;                                    // 512
	st6::wstring description;                             // 528
	uint descripStrId;                                    // 544
	uint datetimeHigh;                                    // 548
	uint datetimeLow;                                     // 552
	uint shipHash;                                        // 556
	int money;                                            // 560
	int numOfKills;                                       // 564
	int numOfSuccessMissions;                             // 568
	int numOfFailedMissions;                              // 572
	float hullStatus;                                     // 576
	st6::list<EquipDesc> currentEquipAndCargo;            // 580
	st6::list<CollisionGroupDesc> currentCollisionGroups; // 592
	float baseHullStatus;                                 // 604
	st6::list<EquipDesc> baseEquipAndCargo;               // 608
	st6::list<CollisionGroupDesc> baseCollisionGroups;    // 620
	uint currentBase;                                     // 632
	uint lastDockedBase;                                  // 636
	uint currentRoom;                                     // 640
	uint system;                                          // 644
	Vector pos;                                           // 648 - 656
	Matrix rot;                                           // 660 - 692
	uint startingRing;                                    // 696
	int rank;                                             // 700
	st6::vector<Reputation::Relation> repList;            // 704
	uint affiliation;                                     // 720, see Reputation::get_id();
	Costume commCostume;                                  // 724 - 772
	uint voiceLen;                                        // 776
	char voice[32] = "trent_voice";                       // 780
	Costume baseCostume;                                  // 812 - 860
	SubObjectID::EquipIdMaker equipIdEnumerator;          // 864
	st6::string prefilledWeaponGroupIni;                  // 876
	st6::list<uint> logInfo;                              // 888
	int interfaceState = 3;                               // 896
	FlMap<uint, char> visits;                             // 900
};

struct PlayerData
{
	wchar_t accId[40];
	FlMap<CHARACTER_ID, CharacterData> characterMap;
	CHARACTER_ID charFile;
	uint iShipArchetype;
	float fRelativeHealth;
	st6::list<CollisionGroupDesc> collisionGroupDesc;
	EquipDescList equipDescList;
	int iRank;
	int iMoneyNeededToNextRank;
	Costume commCostume;
	uint voiceLen;
	char voice[32];
	Costume baseCostume;
	uint iReputation;
	int iInspectCash;
	int worth;
	uint shipArchetypeWhenLanding;
	EquipDescList lShadowEquipDescList;
	int iNumKills;
	int iNumMissionSuccesses;
	int iNumMissionFailures;
	bool skipAutoSave;
	uint saveCount;
	uint iOnlineID;
	bool cheated;
	Vector vPosition;
	Matrix mOrientation;
	st6::string weaponGroups;
	uint dunno1[2];
	int* SPNeuralNetLogUnk;
	int interfaceState;
	uint visitEntriesBST[5];
	//BinarySearchTree<VisitEntry> visitEntries;
	uint dunno2[4]; // last one seems to always be zero
	float difficulty;
	ushort lastEquipId;
	CPlayerTradeOffer* tradeOffer;
	uint onlineId2;
	uint dunno3[2];
	uint tradeRequestCount;
	uint iSystemID;
	uint iShipID;
	uint createdShipId;
	uint iBaseID;
	uint iLastBaseID;
	uint enteredBase;
	uint iBaseRoomID;
	uint iCharacterID;
	class CAccount* Account;
	class CPlayerGroup* PlayerGroup;
	uint iMissionID;
	uint iMissionSetBy;
	uint exitedBase;
	uint unknownLocId;
};

struct SCreateCharacterInfo
{
	wchar_t wszCharname[24];
	uint iNickName; // From [Faction] section of newcharacter.ini
	uint iBase;     // From [Faction] section of newcharacter.ini
	uint iPackage;  // From [Faction] section of newcharacter.ini
	uint iPilot;    // From [Faction] section of newcharacter.ini
	uint iDunno[96];
};

struct SStartupInfo
{
	uint iDunno[130];
	int iMaxPlayers;
};

struct SLoginInfo
{
	wchar_t wszAccount[36];
};

struct PlayerDBTreeNode
{
	PlayerDBTreeNode* pLeft;
	PlayerDBTreeNode* pParent;
	PlayerDBTreeNode* pRight;
	ulong l1;
	// File name of character
	char* szFLName;
	// Length of file name
	uint iLength;
	// Always seems to be 0x1F. Possibly max length of szFLName
	uint iDunno;
	// Account for this player
	CAccount* acc;
};

class IMPORT PlayerDB
{
public:
	PlayerDB(class PlayerDB const&);
	PlayerDB(void);
	~PlayerDB(void);
	class PlayerDB& operator=(class PlayerDB const&);
	struct PlayerData& operator[](unsigned int const&);
	bool BanAccount(st6::wstring&, bool);
	void BuildLocalUserDir(void);
	unsigned int CountPlayersInSystem(int);
	bool CreateAccount(st6::wstring&);
	void DeleteAccount(st6::wstring&);
	void DeleteCharacterFromID(st6::wstring&);
	bool DeleteCharacterFromName(st6::wstring&);
	class CAccount* FindAccountFromCharacterID(st6::string&);
	class CAccount* FindAccountFromCharacterName(st6::wstring&);
	class CAccount* FindAccountFromClientID(unsigned int);
	class CAccount* FindAccountFromName(st6::wstring&);
	bool GetAccountAdminRights(st6::wstring&);
	bool GetAccountBanned(st6::wstring&);
	unsigned short const* GetActiveCharacterName(unsigned int)const;
	bool GetCharactersForAccount(st6::wstring&, class st6::list<st6::wstring>&);
	unsigned int GetGroupID(unsigned int);
	st6::wstring& GetMOTD(void);
	unsigned int GetMaxPlayerCount(void);
	unsigned int GetServerID(void);
	char const* GetServerSig(void);
	void LockAccountAccess(st6::wstring&);
	bool MakeLocalUserPath(char*, char const*);
	void ReadCharacterName(char const*, st6::wstring&);
	void SendGroupID(unsigned int, unsigned int);
	void SendSystemID(unsigned int, unsigned int);
	bool SetAccountAdminRights(st6::wstring&, bool);
	bool SetAccountPassword(st6::wstring&, st6::wstring&);
	void SetMOTD(st6::wstring&);
	void UnlockAccountAccess(st6::wstring&);
	void cleanup(unsigned int);
	bool create_new_character(struct SCreateCharacterInfo const&, unsigned int);
	bool create_restart_file(char const*);
	void free(void);
	void init(unsigned int, bool);
	bool is_valid(unsigned int const&);
	bool is_valid_ship_owner(unsigned int const&, unsigned int const&);
	unsigned char login(struct SLoginInfo const&, unsigned int);
	void logout(unsigned int);
	void logout_all(void);
	struct PlayerData* traverse_active(struct PlayerData*)const;

private:
	int create_account(struct SLoginInfo const&);
	unsigned char load_user_data(struct SLoginInfo const&, unsigned int);
	unsigned int to_index(unsigned int);

public:
	uint iDunno1[13];
	PlayerDBTreeNode* pFirstNode;
	PlayerDBTreeNode* pLastNode;
	uint iDunno2;
	uint iNumAccounts;
};


namespace CmnAsteroid  // from FLCoreCommon.h
{
	class CAsteroidSystem;
};

namespace SrvAsteroid
{
	class IMPORT SrvAsteroidSystem
	{
	public:
		SrvAsteroidSystem(SrvAsteroidSystem const&);
		SrvAsteroidSystem(void);
		~SrvAsteroidSystem(void);
		SrvAsteroidSystem& operator=(SrvAsteroidSystem const&);
		int AddRef(void);
		int Release(void);
		void load(char const*);
		void map_asteroid_fields(void);
		void set_cmn_system(class CmnAsteroid::CAsteroidSystem*);
		void set_sys_id(unsigned int);
		void update(void);

	public:
		unsigned char data[OBJECT_DATA_SIZE];
	};

};

namespace Controller
{
	struct TimerExpired;
}

template <class T> class IMPORT OwnerList
{
public:
	OwnerList<T>();
	virtual ~OwnerList<T>();
	class OwnerList<T>& operator=(class OwnerList<T> const&);
	void free();
};

namespace pub
{
	struct CargoEnumerator;

	IMPORT  int BuildBaseReader(class INI_Reader&, unsigned int const&);
	IMPORT  int BuildSystemReader(class INI_Reader&, unsigned int const&);
	IMPORT  struct HINSTANCE__* DLL_LoadLibrary(char const*);
	IMPORT  void DebugPrint(char const*, int);
	IMPORT  int FindHardpoint(char const*, unsigned int, class Vector&, class Matrix&);
	IMPORT  int GetBaseID(unsigned int&, char const*);
	IMPORT  unsigned int GetBaseNickname(char*, unsigned int, unsigned int const&);
	IMPORT  int GetBaseStridName(unsigned int&, unsigned int const&);
	IMPORT  int GetBases(unsigned int const&, unsigned int* const, unsigned int, unsigned int&);
	IMPORT  int GetCargoHoldSize(unsigned int const&, unsigned int&);
	IMPORT  int GetCostumeID(int&, char const*);
	IMPORT  struct IFileSystem* GetDataPath(void);
	IMPORT  int GetEquipmentID(unsigned int&, char const*);
	IMPORT  int GetFullHealth(unsigned int const&, unsigned int&);
	IMPORT  int GetGoodID(unsigned int&, char const*);
	IMPORT  int GetGoodProperties(unsigned int const&, float&, float&);
	IMPORT  int GetLoadout(struct EquipDescVector&, unsigned int const&);
	IMPORT  int GetLoadoutID(unsigned int&, char const*);
	IMPORT  int GetLoadoutName(unsigned int const&, char*, int);
	IMPORT  unsigned int GetLocationNickname(char*, unsigned int, unsigned int const&);
	IMPORT  int GetLocations(unsigned int const&, unsigned int* const, unsigned int, unsigned int&);
	IMPORT  int GetMaxHitPoints(unsigned int const&, int&);
	IMPORT  int GetNavMapScale(unsigned int, float&);
	IMPORT  unsigned int GetNicknameId(char const*);
	IMPORT  int GetRwTime(double&);
	IMPORT  int GetShipArchSTRID(unsigned int const&, unsigned int&);
	IMPORT  int GetShipID(unsigned int&, char const*);
	IMPORT  int GetSolarType(unsigned int const&, unsigned int&);
	IMPORT  int GetSystem(unsigned int&, unsigned int const&);
	IMPORT  int GetSystemGateConnection(unsigned int const&, unsigned int&);
	IMPORT  int GetSystemID(unsigned int&, char const*);
	IMPORT  unsigned int GetSystemNickname(char*, unsigned int, unsigned int const&);
	IMPORT  int GetTime(double&);
	IMPORT  int GetType(unsigned int const&, unsigned int&);
	IMPORT  int GetVoiceID(unsigned int&, char const*);
	IMPORT  int IsCommodity(unsigned int const&, bool&);
	IMPORT  unsigned short MakeId(char const*);
	IMPORT  bool NextBaseID(unsigned int&);
	IMPORT  bool NextSystemID(unsigned int&);
	IMPORT  int ReportFreeTerminal(unsigned int, int);
	IMPORT  int Save(unsigned int, unsigned int);
	IMPORT  int SetTimer(unsigned int const&, struct Controller::TimerExpired const&, float);
	IMPORT  bool SinglePlayer(void);
	IMPORT  int TranslateArchToGood(unsigned int const&, unsigned int&);
	IMPORT  int TranslateGoodToMsgIdPrefix(unsigned int, struct TString<64>&);
	IMPORT  int TranslateShipToMsgIdPrefix(unsigned int, struct TString<64>&);
	IMPORT  int TranslateSystemToMsgIdPrefix(unsigned int, struct TString<64>&);

	namespace AI
	{
		class Personality;

		IMPORT  enum OP_RTYPE  SubmitDirective(unsigned int, class BaseOp*);
		IMPORT  enum OP_RTYPE  SubmitState(unsigned int, class BaseOp*);
		IMPORT  bool enable_all_maneuvers(unsigned int);
		IMPORT  bool enable_maneuver(unsigned int, int, bool);
		IMPORT  int get_behavior_id(unsigned int);
		IMPORT  bool get_personality(unsigned int, class Personality&);
		IMPORT  enum ScanResponse  get_scan_response(unsigned int, unsigned int, unsigned int);
		IMPORT  int get_state_graph_id(unsigned int);
		IMPORT  bool lock_maneuvers(unsigned int, bool);
		IMPORT  void refresh_state_graph(unsigned int);
		IMPORT  int remove_forced_target(unsigned int, unsigned int);
		IMPORT  enum OP_RTYPE  set_directive_priority(unsigned int, enum DirectivePriority);
		IMPORT  bool set_player_enemy_clamp(unsigned int, int, int);
		IMPORT  int submit_forced_target(unsigned int, unsigned int);
		IMPORT  enum FORMATION_RTYPE  update_formation_state(unsigned int, unsigned int, class Vector const&);
	};

	namespace Audio
	{
		struct Tryptich {
			uint spaceMusic = 0;
			uint dangerMusic = 0;
			uint battleMusic = 0;
			uint overrideMusic = 0;
			bool playOnce = false;
			float crossFadeDurationInS = 0.0f;
		};

		IMPORT  int CancelMusic(unsigned int);
		IMPORT  int PlaySoundEffect(unsigned int, unsigned int);
		IMPORT  int SetMusic(unsigned int, struct Tryptich const&);
	};

	namespace Controller
	{
		struct CreateParms
		{
			void* pFreeFunc;
			uint iClientID;
		};

		enum PRIORITY;

		IMPORT  unsigned int Create(char const*, char const*, struct CreateParms const*, enum PRIORITY);
		IMPORT  void Destroy(unsigned int);
		IMPORT  int SetHeartbeatInterval(unsigned int const&, float);
		IMPORT  int _SendMessage(unsigned int const&, int, void const*);
	};

	namespace GF
	{
		enum class MissionType : uint
		{
			Unknown = 0,
			DestroyShips = 1,
			DestroyInstallation = 2,
			Assassinate = 3,
			DestroyContraband = 4,
			CapturePrisoner = 5,
			RetrieveContraband = 6
		};

		IMPORT  unsigned long AmbientScriptCreate(struct AmbientScriptDescription const&);
		IMPORT  void AmbientScriptDestroy(unsigned long*);
		IMPORT  unsigned long CharacterBehaviorCreate(struct CharacterBehaviorDescription const&);
		IMPORT  void CharacterBehaviorDestroy(unsigned long*);
		IMPORT  unsigned int CharacterCreate(struct CharacterDescription const&);
		IMPORT  void CharacterDestroy(unsigned int*);
		IMPORT  void CharacterSetBehavior(unsigned int, unsigned long);
		IMPORT  void EnumerateCharacterPlacementIni(unsigned int, void(*)(int, class INI_Reader*, void*), void*);
		IMPORT  unsigned int FindBase(char const*);
		IMPORT  unsigned int FindLocation(unsigned int, char const*);
		IMPORT  char const* FormCharacterPlacementName(struct SetpointProperties const*);
		IMPORT  int GetAccessory(char const*);
		IMPORT  int GetBasePosition(unsigned int const&, unsigned int const&, class Vector&);
		IMPORT  int GetBodyPart(char const*, int);
		IMPORT  unsigned int GetCharacterOnPlacement(unsigned int, unsigned int, int);
		IMPORT  int GetCharacterPlacementByName(unsigned int, char const*, int&);
		IMPORT  char const* GetCharacterPlacementName(unsigned int, int);
		IMPORT  unsigned long GetCharacterPlacementOccupancy(unsigned int, int);
		IMPORT  int GetCharacterPlacementPosture(unsigned int, int, unsigned long&);
		IMPORT  bool GetCharacterPlacementProperties(unsigned int, int, struct SetpointProperties*);
		IMPORT  void GetCostumeByID(int, struct Costume&);
		IMPORT  int GetCostumeSkeletonGender(struct Costume const&, int&);
		IMPORT  int GetMissionVendorOfferCount(unsigned int, unsigned int);
		IMPORT  int GetNumCharacterPlacements(unsigned int);
		IMPORT  float GetRtcPerformanceSlider(void);
		IMPORT  unsigned int GetSpaceflightLocation(void);
		IMPORT  bool IsCharacterPlacementNormal(unsigned int, int);
		IMPORT  bool IsCharacterPlacementSpecial(unsigned int, int);
		IMPORT  void MissionVendorAcceptance(unsigned long, bool, struct FmtStr const&, unsigned int);
		IMPORT  unsigned long MissionVendorOfferCreate(struct MissionVendorOfferDescription const&);
		IMPORT  void MissionVendorOfferDestroy(unsigned long*);
		IMPORT  unsigned long NewsBroadcastCreate(struct NewsBroadcastDescription const&);
		IMPORT  void NewsBroadcastDestroy(unsigned long*);
		IMPORT  int ReportWhyMissionVendorEmpty(unsigned int, enum MVEmptyReason);
	};

	namespace Market
	{
		IMPORT  int GetCommoditiesForSale(unsigned int, unsigned int* const, int*);
		IMPORT  int GetCommoditiesInDemand(unsigned int, unsigned int* const, int*);
		IMPORT  int GetGoodJumpDist(unsigned int, unsigned int&);
		IMPORT  int GetMinInventory(unsigned int, unsigned int, int&);
		IMPORT  int GetNominalPrice(unsigned int, float&);
		IMPORT  int GetNumCommoditiesForSale(unsigned int, int*);
		IMPORT  int GetNumCommoditiesInDemand(unsigned int, int*);
		IMPORT  int GetPrice(unsigned int, unsigned int, float&);
		IMPORT  int IsGoodInDemand(unsigned int, unsigned int, bool&);
	};

	namespace Phantom
	{
		IMPORT  int Attach(unsigned int const&, void*);
		IMPORT  int Create(unsigned int, class Vector const&, class Vector const&, class Matrix const&, unsigned int, void*&);
		IMPORT  int Create(unsigned int, float, class Vector const&, unsigned int, void*&);
		IMPORT  int Destroy(void*);
		IMPORT  int Detach(void*);
		IMPORT  int SetActive(void*, bool);
	};

	namespace Player
	{
		enum MissionObjectiveType : uint
		{
			SimpleEntry          = 0,               // Non-objective entry, without waypoint on map
			IntermediateWaypoint = (1 << 3) | (1 << 0), // No entry, with intermediate waypoint on map
			ObjectiveWaypoint    = (1 << 3) | (1 << 1), // Objective entry, with numbered waypoint on map
			MissionText          = 1 << 4,          // Displays message on screen
			ActiveLog            = 1 << 5           // Puts message as log entry
		}; // Bits 3, 7 and 8 are unused.

		struct MissionObjective
		{
			uint type = MissionObjectiveType::IntermediateWaypoint;
			FmtStr message = FmtStr(0, 0);
		};

		IMPORT  int AddCargo(unsigned int const&, unsigned int const&, unsigned int, float, bool);
		IMPORT  int AdjustCash(unsigned int const&, int);
		IMPORT  int CfgInterfaceNotification(unsigned int, unsigned int, bool, int);
		IMPORT  int DisplayMissionMessage(unsigned int const&, struct FmtStr const&, enum MissionMessageType, bool);
		IMPORT  int EnumerateCargo(unsigned int const&, struct pub::CargoEnumerator&);
		IMPORT  int ForceLand(unsigned int, unsigned int);
		IMPORT  int GetAssetValue(unsigned int const&, float&);
		IMPORT  int GetBase(unsigned int const&, unsigned int&);
		IMPORT  int GetBody(unsigned int const&, unsigned int&);
		IMPORT  int GetCharacter(unsigned int const&, unsigned int&);
		IMPORT  int GetGender(unsigned int const&, int&);
		IMPORT  int GetGroupMembers(unsigned int, st6::vector<unsigned int>&);
		IMPORT  int GetGroupSize(unsigned int, unsigned int&);
		IMPORT  int GetLocation(unsigned int const&, unsigned int&);
		IMPORT  int GetMoneyNeededToNextRank(unsigned int const&, int&);
		IMPORT  int GetMsnID(unsigned int clientId, unsigned int& missionId);
		IMPORT  int GetName(unsigned int, st6::wstring&);
		IMPORT  int GetNumKills(unsigned int const&, int&);
		IMPORT  int GetNumMissionFailures(unsigned int const&, int&);
		IMPORT  int GetNumMissionFailures(unsigned int const&, int&);
		IMPORT  int GetNumMissionSuccesses(unsigned int const&, int&);
		IMPORT  int GetRank(unsigned int const&, int&);
		IMPORT  int GetRelativeHealth(unsigned int const&, float&);
		IMPORT  int GetRemainingHoldSize(unsigned int const&, float&);
		IMPORT  int GetRep(unsigned int const&, int&);
		IMPORT  int GetShip(unsigned int const&, unsigned int&);
		IMPORT  int GetShipID(unsigned int const&, unsigned int&);
		IMPORT  int GetSystem(unsigned int const&, unsigned int&);
		IMPORT  int InspectCash(unsigned int const&, int&);
		IMPORT  int IsGroupMember(unsigned int, unsigned int, bool&);
		IMPORT  int LoadHint(unsigned int, struct BaseHint*);
		IMPORT  int MarkObj(unsigned int, unsigned int, int);
		IMPORT  int PopUpDialog(unsigned int, struct FmtStr const&, struct FmtStr const&, unsigned int);
		IMPORT  int RemoveCargo(unsigned int const&, unsigned short, unsigned int);
		IMPORT  int RemoveFromGroup(unsigned int);
		IMPORT  int ReplaceMissionObjective(unsigned int const&, unsigned int const&, unsigned int, struct MissionObjective const&);
		IMPORT  int ReturnBestPath(unsigned int, unsigned char*, int);
		IMPORT  int ReturnPlayerStats(unsigned int, unsigned char*, int);
		IMPORT  int RevertCamera(unsigned int);
		IMPORT  int RewardGroup(unsigned int, int);
		IMPORT  int SendNNMessage(unsigned int, unsigned int);
		IMPORT  int SetCamera(unsigned int, class Transform const&, float, float);
		IMPORT  int SetCostume(unsigned int const&, int);
		IMPORT  int SetInitialOrnt(unsigned int const&, class Matrix const&);
		IMPORT  int SetInitialPos(unsigned int const&, class Vector const&);
		IMPORT  int SetMissionObjectiveState(unsigned int const&, unsigned int const&, int, unsigned int);
		IMPORT  int SetMissionObjectives(uint const& clientId, uint const& objectivesId, struct MissionObjective const* objectiveStructure, uint objectivesEntries, struct FmtStr const& missionType, uchar unk, struct FmtStr const& missionDescription);
		IMPORT  int SetMoneyNeededToNextRank(unsigned int, int);
		IMPORT  int SetMonkey(unsigned int);
		IMPORT  int SetMsnID(unsigned int clientId, unsigned int missionId, unsigned int acceptedByClientId, bool saveGame, unsigned int saveGameName);
		IMPORT  int SetNumKills(unsigned int const&, int);
		IMPORT  int SetNumMissionFailures(unsigned int const&, int);
		IMPORT  int SetNumMissionSuccesses(unsigned int const&, int);
		IMPORT  int SetRank(unsigned int, int);
		IMPORT  int SetRobot(unsigned int);
		IMPORT  int SetShipAndLoadout(unsigned int const&, unsigned int, struct EquipDescVector const&);
		IMPORT  int SetStoryCue(unsigned int const&, unsigned int);
		IMPORT  int SetTrent(unsigned int);
	};

	namespace Reputation
	{
		IMPORT  int Alloc(int&, struct FmtStr const&, struct FmtStr const&);
		IMPORT  int EnumerateGroups(struct Enumerator&);
		IMPORT  int Free(int const&);
		IMPORT  int GetAffiliation(int const&, unsigned int&);
		IMPORT  int GetAttitude(int const&, int const&, float&);
		IMPORT  int GetGroupFeelingsTowards(int const&, unsigned int const&, float&);
		IMPORT  int GetGroupName(unsigned int const&, unsigned int&);
		IMPORT  int GetName(int const&, struct FmtStr&, struct FmtStr&);
		IMPORT  int GetRank(int const&, float&);
		IMPORT  int GetReputation(int&, struct ID_String const&);
		IMPORT  int GetReputation(int&, char const*);
		IMPORT  int GetReputation(int const&, unsigned int const&, float&);
		IMPORT  int GetReputation(unsigned int const&, unsigned int const&, float&);
		IMPORT  int GetReputationGroup(unsigned int&, char const*);
		IMPORT  int GetShortGroupName(unsigned int const&, unsigned int&);
		IMPORT  int SetAffiliation(int const&, unsigned int const&);
		IMPORT  int SetAttitude(int const&, int const&, float);
		IMPORT  int SetRank(int const&, float);
		IMPORT  int SetReputation(int const&, unsigned int const&, float);
		IMPORT  int SetReputation(unsigned int const&, unsigned int const&, float);
	};

	namespace SpaceObj
	{
		struct CargoDesc
		{
			int vTbl;
			int iUnk1;
			int iUnk2;
			int iUnk3;
			int iUnk4;
		};

		struct ShipInfo
		{
			uint iFlag;
			uint iSystem;
			uint iShipArchetype;
			Vector vPos;
			Vector vAngularVelocity;
			Vector vLinearVelocity;
			Matrix mOrientation;
			uint iGroupId; // 0
			uint iLoadout;
			OwnerList<pub::SpaceObj::CargoDesc>* cargoDesc;
			uint unk1; // 0
			uint unk2; // 0
			OwnerList<pub::SpaceObj::CargoDesc>* cargoDesc2;
			uint unk3; // 0
			Costume Costume;
			int iRep;
			uint iPilotVoice;
			uint DockTargetId; // 0
			int iHitPointsLeft; // -1 = max health
			uint unk7; // 0
			uint unk8; // 0
			uint iLevel;
		};

		struct SolarInfo {
			int iFlag; //0x290; ShipInfo has this too, no clue whether actually a flag
			uint iArchID;
			uint iSystemID;
			Vector vPos;
			Matrix mOrientation;
			uint iLoadoutID;
			Costume Costume;
			int iRep;
			int iVoiceID;
			uint baseId;
			bool missionBool;
			int iHitPointsLeft;
			char cNickName[64]; //Has to be unique
			uint iUnk11; //0 unused?
			bool missionMarked; // 1 = flagged as mission solar, 0 = normal
		};

		struct LootInfo {
			uint systemId;
			uint equipmentArchId;
			uint infocardOverride;
			uint ownerId;
			Vector initialVelocity;
			Vector pos;
			Matrix rot;
			Vector initialAngular;
			int itemCount;
			float hitPtsPercentage;
			bool canAITractor;
			bool isMissionLoot;
		};

		struct TerminalInfo {
			char szHardPoint[0x20];
			uint iType; // 1=berth 4=moor? 7=jump?
		};

		IMPORT  int Activate(unsigned int const&, bool, int);
		IMPORT  enum EQUIPMENT_RTYPE  ActivateEquipment(unsigned int const&, struct EQInfo*);
		IMPORT  int AddImpulse(unsigned int const&, class Vector const&, class Vector const&);
		IMPORT  int Create(unsigned int&, struct ShipInfo const&);
		IMPORT  int CreateLoot(unsigned int&, struct LootInfo const&);
		IMPORT  int CreateSolar(unsigned int&, struct SolarInfo const&);
		IMPORT  int Destroy(unsigned int, enum DestroyType);
		IMPORT  int Dock(unsigned int const&, unsigned int const&, int, enum DOCK_HOST_RESPONSE);
		IMPORT  int DockRequest(unsigned int const&, unsigned int const&);
		IMPORT  int DrainShields(unsigned int);
		IMPORT  int EnumerateCargo(unsigned int const&, struct pub::CargoEnumerator&);
		IMPORT  int ExistsAndAlive(unsigned int); // returns 0 for alive, -2 for dead
		IMPORT  int FormationResponse(unsigned int const&, enum FORMATION_RTYPE);
		IMPORT  int GetArchetypeID(unsigned int const&, unsigned int&);
		IMPORT  int GetAtmosphereRange(unsigned int const&, float&);
		IMPORT  int GetBurnRadius(unsigned int const&, float&, class Vector&);
		IMPORT  int GetCargoSpaceOccupied(unsigned int const&, unsigned int&);
		IMPORT  int GetCenterOfMass(unsigned int const&, class Vector&);
		IMPORT  int GetDockingTarget(unsigned int const&, unsigned int&);
		IMPORT  int GetEmptyPos(unsigned int const&, class Transform const&, float const&, float const&, enum PosSelectionType const&, class Vector&);
		IMPORT  int GetGoodID(unsigned int const&, unsigned int&);
		IMPORT  int GetHardpoint(unsigned int const&, char const*, class Vector*, class Matrix*);
		IMPORT  int GetHealth(unsigned int const& iSpaceObj, float& fCurrentHealth, float& fMaxHealth);
		IMPORT  int GetInvincible(unsigned int, bool&, bool&, float&);
		IMPORT  int GetJumpTarget(unsigned int const&, unsigned int&, unsigned int&);
		IMPORT  int GetLocation(unsigned int, class Vector&, class Matrix&);
		IMPORT  int GetMass(unsigned int const&, float&);
		IMPORT  int GetMotion(unsigned int, class Vector&, class Vector&);
		IMPORT  int GetRadius(unsigned int const&, float&, class Vector&);
		IMPORT  int GetRelativeHealth(unsigned int const&, float&);
		IMPORT  int GetRep(unsigned int, int&);
		IMPORT  int GetScannerRange(unsigned int, int&, int&);
		IMPORT  int GetShieldHealth(unsigned int const& iSpaceObj, float& fCurrentShields, float& fMaxShields, bool& bShieldsUp);
		IMPORT  int GetSolarArchetypeID(unsigned int, unsigned int&);
		IMPORT  int GetSolarArchetypeNickname(char*, int, unsigned int);
		IMPORT  int GetSolarParent(unsigned int const&, unsigned int&);
		IMPORT  int GetSolarRep(unsigned int, int&);
		IMPORT  int GetSystem(unsigned int, unsigned int&);
		IMPORT  int GetTarget(unsigned int const&, unsigned int&);
		IMPORT  int GetTerminalInfo(unsigned int const&, int, struct TerminalInfo&);
		IMPORT  int GetTradelaneNextAndPrev(unsigned int const&, unsigned int&, unsigned int&);
		IMPORT  int GetType(unsigned int, unsigned int&);
		IMPORT  int GetVoiceID(unsigned int const&, unsigned int&);
		IMPORT  int InstantDock(unsigned int const&, unsigned int const&, int);
		IMPORT  int IsPosEmpty(unsigned int const&, class Vector const&, float const&, bool&);
		IMPORT  int JettisonEquipment(unsigned int const&, unsigned short const&, int const&);
		IMPORT  int JumpIn(unsigned int const&, unsigned int const&);
		IMPORT  int LaneResponse(unsigned int const&, int);
		IMPORT  int Launch(unsigned int const& launcherId, unsigned int const& launchTargetId, int dockId);
		IMPORT  int LightFuse(unsigned int const&, char const*, float);
		IMPORT  int Relocate(unsigned int const&, unsigned int const&, class Vector const&, class Matrix const&);
		IMPORT  int RequestSpaceScript(unsigned int const&, class Vector const&, int const&, unsigned int, char const*);
				// "sender" will always set the name. If "sender" is zero, "name" is used. "commType" is hardcoded values, but 0 works, too.
		IMPORT  int SendComm(uint senderObjId, uint receiverObjId, uint voice, const Costume* costume, uint name, uint* lines, int lineCount, uint commType, float radioSilenceTimeAfter, bool global);
		IMPORT  int SetInvincible2(unsigned int spaceObjectId,bool preventNpcDamage,bool preventPlayerDamage,float maxHpLossPercentage);
		IMPORT  int SetInvincible(unsigned int spaceObjectId, bool preventDamage, bool allowPlayerDamage, float maxHpLossPercentage);
		IMPORT  int SetRelativeHealth(unsigned int const&, float);
	};

	namespace System
	{
		struct SysObj
		{
			char   nick[32];		// NOT NUL-terminated if longer
			Vector pos;
			UINT   archid;
			UINT   ids_name;
			UINT   ids_info;
			char   reputation[32];
			UINT   dock_with;
			UINT   goto_system;
			UINT   system;
			// -------------------
			// Some nicknames are longer than 32 characters, so take advantage of the
			// fact that where it gets the nickname from immediately follows.
			size_t len;		// TString<64>
			char   nickname[64];
		};

		struct SysObjEnumerator
		{
			virtual bool operator()(const SysObj&) = 0;
		};

		struct Connection
		{
			UINT   stuff[10];
		};

		struct ConnectionEnumerator
		{
			virtual bool operator()(const Connection&) = 0;
		};

		IMPORT  int EnumerateConnections(unsigned int const&, struct pub::System::ConnectionEnumerator&, enum ConnectionType);
		IMPORT  int EnumerateObjects(unsigned int const&, struct SysObjEnumerator&);
		IMPORT  int EnumerateZones(unsigned int const&, struct ZoneEnumerator&);
		IMPORT  int Find(unsigned int const&, char const*, unsigned int&);
		IMPORT  int GetName(unsigned int, unsigned int&);
		IMPORT  int GetNestedProperties(unsigned int const&, class Vector const&, unsigned long&);
		IMPORT  int InZones(unsigned int, class Transform const&, float, float, float, unsigned int* const, unsigned int, unsigned int&);
		IMPORT  int InZones(unsigned int, class Vector const&, float, unsigned int* const, unsigned int, unsigned int&);
		IMPORT  int LoadSystem(unsigned int const&);
		IMPORT  int ScanObjects(unsigned int const&, unsigned int* const, unsigned int, class Vector const&, float, unsigned int, unsigned int&);
	};

	namespace Zone
	{
		IMPORT  float ClassifyPoint(unsigned int, class Vector const&);
		IMPORT  float GetDistance(unsigned int, class Vector const&);
		IMPORT  unsigned int GetId(unsigned int, char const*);
		IMPORT  int GetLootableInfo(unsigned int, struct ID_String&, struct ID_String&, int&, int&, int&);
		IMPORT  int GetName(unsigned int, unsigned int&);
		IMPORT  int GetOrientation(unsigned int const&, class Matrix&);
		//IMPORT  int GetPopulation(unsigned int,class weighted_vector<unsigned int> const * &);
		IMPORT  class Vector  GetPos(unsigned int);
		IMPORT  int GetProperties(unsigned int, unsigned long&);
		IMPORT  float GetRadius(unsigned int);
		IMPORT  int GetShape(unsigned int, enum ZoneShape&);
		IMPORT  int GetSize(unsigned int, class Vector&);
		IMPORT  unsigned int GetSystem(unsigned int);
		IMPORT  bool InZone(unsigned int, class Vector const&, float);
		IMPORT  bool Intersect(unsigned int, class Vector const&, class Vector const&, class Vector&);
		IMPORT  bool VerifyId(unsigned int);
	};

};

IMPORT  void(*g_pPrivateChatHook)(unsigned short*, int);
IMPORT  void(*g_pSystemChatHook)(unsigned short*, int);
IMPORT  void(*g_pUniverseChatHook)(unsigned short*, int);

IMPORT  PlayerDB  Players;
extern "C" IMPORT IServerImpl Server;

#endif // _FLCORESERVER_H_
