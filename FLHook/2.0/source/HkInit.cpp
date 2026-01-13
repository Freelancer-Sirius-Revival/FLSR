#include <process.h>
#include "hook.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

PATCH_INFO piFLServerEXE = {
    "flserver.exe",
    0x0400000,
    {
        {0x041B094, &HkIEngine::Update_Time, 4, 0, false},
        {0x041BAB0, &HkIEngine::Elapse_Time, 4, 0, false},

        {0, 0, 0, 0} // terminate
    }};

PATCH_INFO piContentDLL = {"content.dll",
                           0x6EA0000,
                           {
                               {0x6FB358C, &HkIEngine::Dock_Call, 4, 0, false},

                               {0, 0, 0, 0} // terminate
                           }};

PATCH_INFO piCommonDLL = {"common.dll",
                          0x6260000,
                          {

                              {0x0639C138, &HkIEngine::_CShip_init, 4,
                               &HkIEngine::fpOldInitCShip, false},
                              {0x0639C064, &HkIEngine::_CShip_destroy, 4,
                               &HkIEngine::fpOldDestroyCShip, false},

                              {0, 0, 0, 0} // terminate
                          }};

PATCH_INFO piServerDLL = {
    "server.dll",
    0x6CE0000,
    {
        //{0x6D672A4,		&ApplyShipDamageListNaked,			4, &ApplyShipDamageListOrigFunc,false},
        //{0x6D67F4C,		&LootDestroyedNaked,				4, &LootDestroyedOrigFunc,	false},
        //{0x6D661C4,		&MineDestroyedNaked,				4, &MineDestroyedOrigFunc,		false},
        //{0x6D66694,		&GuidedDestroyedNaked,				4, &GuidedDestroyedOrigFunc,	false},
        {0x6D6733C,		&ShipColGrpDestroyedHookNaked,		4, &ShipAndSolarColGrpDeathOrigFunc,	false},
        {0x6D67340,		&ShipEquipDestroyedHookNaked,		4, &ShipAndSolarEquipDeathOrigFunc,		false},
        {0x6D6768C,		&SolarColGrpDestroyedHookNaked,		4, 0,							false}, // Same original function as ShipColGrpDeath
        {0x6D67690,		&SolarEquipDestroyedHookNaked,		4, 0,							false}, // Same original function as ShipEquipDeath
        {0x6D67274,		&ShipDestroyedNaked,				4, &ShipDestroyedOrigFunc,		false},
        {0x6D672A0,		&ShipExplosionHitNaked,				4, &ShipExplosionHitOrigFunc,	false},
        {0x6D672CC,		&ShipHullDamageNaked,				4, &ShipHullDamageOrigFunc,		false},
        {0x6D6732C,		&ShipEquipDamageNaked,		        4, &ShipEquipDamageOrigFunc,	false},
        {0x6D67330,		&ShipShieldDamageNaked,		        4, &ShipShieldDamageOrigFunc,	false},
        {0x6D67334,		&ShipColGrpDamageNaked,		        4, &ShipColGrpDamageOrigFunc,   false},
        {0x6D675C4,		&SolarDestroyedNaked,				4, &SolarDestroyedOrigFunc,		false},
        {0x6D675F0,		&SolarExplosionHitNaked,            4, &SolarExplosionHitOrigFunc,  false},
        {0x6D6761C,		&SolarHullDamageNaked,				4, &SolarHullDamageOrigFunc,    false},
        {0x6D6767C,		&SolarEquipDamageNaked,             4, &SolarEquipDamageOrigFunc,	false},
        {0x6D67680,		&SolarShieldDamageNaked,            4, &SolarShieldDamageOrigFunc,	false},
        {0x6D67684,		&SolarColGrpDamageNaked,            4, &SolarColGrpDamageOrigFunc,  false},
        {0x6D6420C,		&HkIEngine::_LaunchPos,				4, &HkIEngine::fpOldLaunchPos,	false},
        {0x6D648E0,		&HkIEngine::FreeReputationVibe,		4, 0,							false},

        {0, 0, 0, 0} // terminate
    }};

PATCH_INFO piRemoteClientDLL = {
    "remoteclient.dll",
    0x6B30000,
    {
        {0x6B6BB80, &HkCb_SendChat, 4, &RCSendChatMsg, false},

        {0, 0, 0, 0} // terminate
    }};

PATCH_INFO piDaLibDLL = {
    "dalib.dll",
    0x65C0000,
    {
        {0x65C4BEC, &_DisconnectPacketSent, 4, &fpOldDiscPacketSent, false},

        {0, 0, 0, 0} // terminate
    }};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Patch(PATCH_INFO &pi) {
    HMODULE hMod = GetModuleHandle(pi.szBinName);
    if (!hMod)
        return false;

    for (uint i = 0; (i < sizeof(pi.piEntries) / sizeof(PATCH_INFO_ENTRY));
         i++) {
        if (!pi.piEntries[i].pAddress)
            break;

        char *pAddress =
            (char *)hMod + (pi.piEntries[i].pAddress - pi.pBaseAddress);
        if (!pi.piEntries[i].pOldValue) {
            pi.piEntries[i].pOldValue = new char[pi.piEntries[i].iSize];
            pi.piEntries[i].bAlloced = true;
        } else
            pi.piEntries[i].bAlloced = false;

        ReadProcMem(pAddress, pi.piEntries[i].pOldValue, pi.piEntries[i].iSize);
        WriteProcMem(pAddress, &pi.piEntries[i].pNewValue,
                     pi.piEntries[i].iSize);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RestorePatch(PATCH_INFO &pi) {
    HMODULE hMod = GetModuleHandle(pi.szBinName);
    if (!hMod)
        return false;

    for (uint i = 0; (i < sizeof(pi.piEntries) / sizeof(PATCH_INFO_ENTRY));
         i++) {
        if (!pi.piEntries[i].pAddress)
            break;

        char *pAddress =
            (char *)hMod + (pi.piEntries[i].pAddress - pi.pBaseAddress);
        WriteProcMem(pAddress, pi.piEntries[i].pOldValue,
                     pi.piEntries[i].iSize);
        if (pi.piEntries[i].bAlloced)
            delete[] pi.piEntries[i].pOldValue;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDPClientProxy **g_cClientProxyArray;

CDPServer *cdpSrv;

HkIClientImpl *FakeClient;
HkIClientImpl *HookClient;
char *OldClient;

_CRCAntiCheat CRCAntiCheat;
_CreateChar CreateChar;

std::string scAcctPath;

CLIENT_INFO ClientInfo[MAX_CLIENT_ID + 1];

uint g_iServerLoad = 0;
uint g_iPlayerCount = 0;

char *g_FLServerDataPtr;

_GetShipInspect GetShipInspect;

std::list<BASE_INFO> lstBases;

char szRepFreeFixOld[5];

/**************************************************************************************************************
clear the clientinfo
**************************************************************************************************************/

void ClearClientInfo(uint iClientID) {
    ClientInfo[iClientID].dieMsg = DIEMSG_ALL;
    ClientInfo[iClientID].iShip = 0;
    ClientInfo[iClientID].iShipOld = 0;
    ClientInfo[iClientID].tmSpawnTime = 0;
    ClientInfo[iClientID].lstMoneyFix.clear();
    ClientInfo[iClientID].iTradePartner = 0;
    ClientInfo[iClientID].iBaseEnterTime = 0;
    ClientInfo[iClientID].iCharMenuEnterTime = 0;
    ClientInfo[iClientID].tmKickTime = 0;
    ClientInfo[iClientID].iLastExitedBaseID = 0;
    ClientInfo[iClientID].bDisconnected = false;
    ClientInfo[iClientID].bCharSelected = false;
    ClientInfo[iClientID].tmF1Time = 0;
    ClientInfo[iClientID].tmF1TimeDisconnect = 0;

    ClientInfo[iClientID].dmgLastCause = DamageCause::Unknown;
    DamageList dmg;
    ClientInfo[iClientID].dmgLast = dmg;
    ClientInfo[iClientID].dieMsgSize = CS_DEFAULT;
    ClientInfo[iClientID].chatSize = CS_DEFAULT;
    ClientInfo[iClientID].chatStyle = CST_DEFAULT;

    ClientInfo[iClientID].lstIgnore.clear();
    ClientInfo[iClientID].iKillsInARow = 0;
    ClientInfo[iClientID].wscHostname = L"";
    ClientInfo[iClientID].iGroupID = 0;
    ClientInfo[iClientID].bSpawnProtected = false;

    CALL_PLUGINS_V(PLUGIN_ClearClientInfo, , (uint), (iClientID));
}

/**************************************************************************************************************
load settings from flhookhuser.ini
**************************************************************************************************************/

void LoadUserSettings(uint iClientID) {
    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    std::wstring wscDir;
    HkGetAccountDirName(acc, wscDir);
    std::string scUserFile = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";

    // read diemsg settings
    ClientInfo[iClientID].dieMsg =
        (DIEMSGTYPE)IniGetI(scUserFile, "settings", "DieMsg", DIEMSG_ALL);
    ClientInfo[iClientID].dieMsgSize =
        (CHATSIZE)IniGetI(scUserFile, "settings", "DieMsgSize", CS_DEFAULT);

    // read chatstyle settings
    ClientInfo[iClientID].chatSize =
        (CHATSIZE)IniGetI(scUserFile, "settings", "ChatSize", CS_DEFAULT);
    ClientInfo[iClientID].chatStyle =
        (CHATSTYLE)IniGetI(scUserFile, "settings", "ChatStyle", CST_DEFAULT);

    // read ignorelist
    ClientInfo[iClientID].lstIgnore.clear();
    for (int i = 1;; i++) {
        std::wstring wscIgnore =
            IniGetWS(scUserFile, "IgnoreList", std::to_string(i), L"");
        if (!wscIgnore.length())
            break;

        IGNORE_INFO ii;
        ii.wscCharname = GetParam(wscIgnore, ' ', 0);
        ii.wscFlags = GetParam(wscIgnore, ' ', 1);
        ClientInfo[iClientID].lstIgnore.push_back(ii);
    }
}

void Detour(void* pOFunc, void* pHkFunc)
{
    DWORD dwOldProtection = 0; // Create a DWORD for VirtualProtect calls to allow us to write.
    BYTE bPatch[5]; // We need to change 5 bytes and I'm going to use memcpy so this is the simplest way.
    bPatch[0] = 0xE9; // Set the first byte of the byte array to the op code for the JMP instruction.
    VirtualProtect(pOFunc, 5, PAGE_EXECUTE_READWRITE, &dwOldProtection); // Allow us to write to the memory we need to change
    DWORD dwRelativeAddress = (DWORD)pHkFunc - (DWORD)pOFunc - 5; // Calculate the relative JMP address.
    memcpy(&bPatch[1], &dwRelativeAddress, 4); // Copy the relative address to the byte array.
    memcpy(pOFunc, bPatch, 5); // Change the first 5 bytes to the JMP instruction.
    VirtualProtect(pOFunc, 5, dwOldProtection, 0); // Set the protection back to what it was.
}

/**************************************************************************************************************
load settings from flhookhuser.ini (specific to character)
**************************************************************************************************************/

void LoadUserCharSettings(uint iClientID) {
    CALL_PLUGINS_V(PLUGIN_LoadUserCharSettings, , (uint), (iClientID));
}

/**************************************************************************************************************
install the callback hooks
**************************************************************************************************************/

bool InitHookExports() {
    char *pAddress;

    // init critial sections
    InitializeCriticalSection(&csIPResolve);
    DWORD dwID;
    DWORD dwParam[34]; // else release version crashes, dont ask me why...
    hThreadResolver = CreateThread(
        0, 0, (LPTHREAD_START_ROUTINE)HkThreadResolver, &dwParam, 0, &dwID);

    GetShipInspect = (_GetShipInspect)SRV_ADDR(ADDR_SRV_GETINSPECT);

    // install IServerImpl callbacks in remoteclient.dll
    char *pServer = (char *)&Server;
    memcpy(&pServer, pServer, 4);
    for (uint i = 0;
         (i < sizeof(HkIServerImpl::hookEntries) / sizeof(HOOKENTRY)); i++) {
        char *pAddress =
            pServer + HkIServerImpl::hookEntries[i].dwRemoteAddress;
        ReadProcMem(pAddress, &HkIServerImpl::hookEntries[i].fpOldProc, 4);
        WriteProcMem(pAddress, &HkIServerImpl::hookEntries[i].fpProc, 4);
    }

    // patch it
    Patch(piFLServerEXE);
    Patch(piContentDLL);
    Patch(piCommonDLL);
    Patch(piServerDLL);
    Patch(piRemoteClientDLL);
    Patch(piDaLibDLL);

    // patch rep array free
    char szNOPs[] = {'\x90', '\x90', '\x90', '\x90', '\x90'};
    pAddress = ((char *)hModServer + ADDR_SRV_REPARRAYFREE);
    ReadProcMem(pAddress, szRepFreeFixOld, 5);
    WriteProcMem(pAddress, szNOPs, 5);

    /* GetShipInspect optimizations */
    FARPROC GameObjectDestructor = FARPROC(0x6CEE4A0);
    Detour(GameObjectDestructor, HkIEngine::GameObjectDestructorNaked);

    FARPROC CAsteroidInit = FARPROC(0x62A28F0);
    Detour(CAsteroidInit, HkIEngine::CAsteroidInitNaked);

    FARPROC CObjectDestr = FARPROC(0x62AF440);
    Detour(CObjectDestr, HkIEngine::CObjDestrOrgNaked);

    BYTE patchCobjDestr[] = { 0xEB, 0x5F };
    WriteProcMem((char*)hModCommon + 0x4F45D, patchCobjDestr, sizeof(patchCobjDestr));

    FARPROC CObjectFindDetourFunc = FARPROC(&HkIEngine::CObjectFindDetour);
    WriteProcMem((char*)hModServer + 0x84464, &CObjectFindDetourFunc, 4);

    FARPROC CObjAlloc = FARPROC(0x62AEE50);
    Detour(CObjAlloc, HkIEngine::CObjAllocDetour);

    FARPROC FindIObjInStarList = FARPROC(0x6D0C840);
    Detour(FindIObjInStarList, HkIEngine::FindInStarListNaked);
    /* end GetShipInspect optimizations */


    FARPROC CGuidedInit = FARPROC(0x62ACCB0);
    Detour(CGuidedInit, HkIEngine::CGuidedInitNaked);

    // patch flserver so it can better handle faulty house entries in char files

    // divert call to house load/save func
    pAddress = SRV_ADDR(0x679C6);
    char szDivertJump[] = {'\x6F'};

    WriteProcMem(pAddress, szDivertJump, 1);

    // install hook at new address
    pAddress = SRV_ADDR(0x78B39);

    char szMovEAX[] = {'\xB8'};
    char szJMPEAX[] = {'\xFF', '\xE0'};

    FARPROC fpHkLoadRepFromCharFile =
        (FARPROC)HkIEngine::_HkLoadRepFromCharFile;

    WriteProcMem(pAddress, szMovEAX, 1);
    WriteProcMem(pAddress + 1, &fpHkLoadRepFromCharFile, 4);
    WriteProcMem(pAddress + 5, szJMPEAX, 2);

    HkIEngine::fpOldLoadRepCharFile = (FARPROC)SRV_ADDR(0x78B40);

    // crc anti-cheat
    CRCAntiCheat = (_CRCAntiCheat)((char *)hModServer + ADDR_CRCANTICHEAT);

    // get CDPServer
    pAddress = DALIB_ADDR(ADDR_CDPSERVER);
    ReadProcMem(pAddress, &cdpSrv, 4);

    // read g_FLServerDataPtr(used for serverload calc)
    pAddress = FLSERVER_ADDR(ADDR_DATAPTR);
    ReadProcMem(pAddress, &g_FLServerDataPtr, 4);

    // some setting relate hooks
    HookRehashed();

    // get client proxy array, used to retrieve player pings/ips
    pAddress = (char *)hModRemoteClient + ADDR_CPLIST;
    char *szTemp;
    ReadProcMem(pAddress, &szTemp, 4);
    szTemp += 0x10;
    memcpy(&g_cClientProxyArray, &szTemp, 4);

    // init variables
    char szDataPath[MAX_PATH];
    GetUserDataPath(szDataPath);
    scAcctPath = std::string(szDataPath) + "\\Accts\\MultiPlayer\\";

    // clear ClientInfo
    for (uint i = 0; (i < sizeof(ClientInfo) / sizeof(CLIENT_INFO)); i++) {
        ClientInfo[i].iConnects = 0; // only set to 0 on start
        ClearClientInfo(i);
    }

    return true;
}

void PatchClientImpl() {
    // install HkIClientImpl callback

    FakeClient = new HkIClientImpl;
    HookClient = &Client;

    memcpy(&OldClient, &Client, 4);
    WriteProcMem(&Client, FakeClient, 4);
}

/**************************************************************************************************************
uninstall the callback hooks
**************************************************************************************************************/

void UnloadHookExports() {
    char *pAddress;

    // uninstall IServerImpl callbacks in remoteclient.dll
    char *pServer = (char *)&Server;
    if (pServer) {
        memcpy(&pServer, pServer, 4);
        for (uint i = 0;
             (i < sizeof(HkIServerImpl::hookEntries) / sizeof(HOOKENTRY));
             i++) {
            void *pAddress =
                (void *)((char *)pServer +
                         HkIServerImpl::hookEntries[i].dwRemoteAddress);
            WriteProcMem(pAddress, &HkIServerImpl::hookEntries[i].fpOldProc, 4);
        }
    }

    // reset npc spawn setting
    HkChangeNPCSpawn(false);

    // restore other hooks
    RestorePatch(piFLServerEXE);
    RestorePatch(piContentDLL);
    RestorePatch(piCommonDLL);
    RestorePatch(piServerDLL);
    RestorePatch(piRemoteClientDLL);
    RestorePatch(piDaLibDLL);

    // unpatch rep array free
    pAddress = ((char *)GetModuleHandle("server.dll") + ADDR_SRV_REPARRAYFREE);
    WriteProcMem(pAddress, szRepFreeFixOld, 5);

    // unpatch flserver so it can better handle faulty house entries in char
    // files

    // undivert call to house load/save func
    pAddress = SRV_ADDR(0x679C6);
    char szDivertJump[] = {'\x76'};

    // anti-death-msg
    char szOld[] = {'\x74'};
    pAddress = SRV_ADDR(ADDR_ANTIDIEMSG);
    WriteProcMem(pAddress, szOld, 1);

    // plugins
    PluginManager::UnloadPlugins();
    PluginManager::Destroy();

    // help
    lstHelpEntries.clear();
}

/**************************************************************************************************************
settings were rehashed
sometimes adjustments need to be made after a rehash
**************************************************************************************************************/

void HookRehashed() {
    char *pAddress;

    // anti-deathmsg
    if (set_bDieMsg) { // disables the "old" "A Player has died: ..." messages
        char szJMP[] = {'\xEB'};
        pAddress = SRV_ADDR(ADDR_ANTIDIEMSG);
        WriteProcMem(pAddress, szJMP, 1);
    } else {
        char szOld[] = {'\x74'};
        pAddress = SRV_ADDR(ADDR_ANTIDIEMSG);
        WriteProcMem(pAddress, szOld, 1);
    }

    // charfile encyption(doesn't get disabled when unloading FLHook)
    if (set_bDisableCharfileEncryption) {
        char szBuf[] = {'\x14', '\xB3'};
        pAddress = SRV_ADDR(ADDR_DISCFENCR);
        WriteProcMem(pAddress, szBuf, 2);
        pAddress = SRV_ADDR(ADDR_DISCFENCR2);
        WriteProcMem(pAddress, szBuf, 2);
    } else {
        char szBuf[] = {'\xE4', '\xB4'};
        pAddress = SRV_ADDR(ADDR_DISCFENCR);
        WriteProcMem(pAddress, szBuf, 2);
        pAddress = SRV_ADDR(ADDR_DISCFENCR2);
        WriteProcMem(pAddress, szBuf, 2);
    }

    // maximum group size
    if (set_iMaxGroupSize > 0) {
        char cNewGroupSize = set_iMaxGroupSize & 0xFF;
        pAddress = SRV_ADDR(ADDR_SRV_MAXGROUPSIZE);
        WriteProcMem(pAddress, &cNewGroupSize, 1);
        pAddress = SRV_ADDR(ADDR_SRV_MAXGROUPSIZE2);
        WriteProcMem(pAddress, &cNewGroupSize, 1);
    } else { // default
        char cNewGroupSize = 8;
        pAddress = SRV_ADDR(ADDR_SRV_MAXGROUPSIZE);
        WriteProcMem(pAddress, &cNewGroupSize, 1);
        pAddress = SRV_ADDR(ADDR_SRV_MAXGROUPSIZE2);
        WriteProcMem(pAddress, &cNewGroupSize, 1);
    }

    // open debug log if necessary
    if (set_bDebug && !fLogDebug) {
        fopen_s(&fLogDebug, sDebugLog.c_str(), "at");
    } else if (!set_bDebug && fLogDebug) {
        fclose(fLogDebug);
        fLogDebug = nullptr;
    }
}
