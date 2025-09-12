#include "hook.h"

/**************************************************************************************************************
**************************************************************************************************************/

std::wstring SetSizeToSmall(const std::wstring& wscDataFormat)
{
    uint iFormat = wcstoul(wscDataFormat.c_str() + 2, 0, 16);
    wchar_t wszStyleSmall[32];
    wcscpy_s(wszStyleSmall, wscDataFormat.c_str());
    swprintf(wszStyleSmall + wcslen(wszStyleSmall) - 2, L"%02X", 0x90 | (iFormat & 7));
    return wszStyleSmall;
}

/**************************************************************************************************************
Send "Death: ..." chat-message
**************************************************************************************************************/

void SendDeathMsg(const std::wstring& wscMsg, uint iSystemID, uint iClientIDVictim, uint iClientIDKiller, DamageCause deathCause)
{

    CALL_PLUGINS_V(PLUGIN_SendDeathMsg, , (const std::wstring&, uint, uint, uint), (wscMsg, iSystemID, iClientIDVictim, iClientIDKiller));

    // encode xml string(default and small)
    // non-sys
    std::wstring wscXMLMsg = L"<TRA data=\"" + set_wscDeathMsgStyle + L"\" mask=\"-1\"/> <TEXT>";
    wscXMLMsg += XMLText(wscMsg);
    wscXMLMsg += L"</TEXT>";

    char szBuf[0xFFFF];
    uint iRet;
    if (!HKHKSUCCESS(HkFMsgEncodeXML(wscXMLMsg, szBuf, sizeof(szBuf), iRet)))
        return;

    std::wstring wscStyleSmall = SetSizeToSmall(set_wscDeathMsgStyle);
    std::wstring wscXMLMsgSmall = std::wstring(L"<TRA data=\"") + wscStyleSmall + L"\" mask=\"-1\"/> <TEXT>";
    wscXMLMsgSmall += XMLText(wscMsg);
    wscXMLMsgSmall += L"</TEXT>";
    char szBufSmall[0xFFFF];
    uint iRetSmall;
    if (!HKHKSUCCESS(HkFMsgEncodeXML(wscXMLMsgSmall, szBufSmall, sizeof(szBufSmall), iRetSmall)))
        return;

    // sys
    std::wstring wscXMLMsgSys = L"<TRA data=\"" + set_wscDeathMsgStyleSys + L"\" mask=\"-1\"/> <TEXT>";
    wscXMLMsgSys += XMLText(wscMsg);
    wscXMLMsgSys += L"</TEXT>";
    char szBufSys[0xFFFF];
    uint iRetSys;
    if (!HKHKSUCCESS(HkFMsgEncodeXML(wscXMLMsgSys, szBufSys, sizeof(szBufSys), iRetSys)))
        return;

    std::wstring wscStyleSmallSys = SetSizeToSmall(set_wscDeathMsgStyleSys);
    std::wstring wscXMLMsgSmallSys = L"<TRA data=\"" + wscStyleSmallSys + L"\" mask=\"-1\"/> <TEXT>";
    wscXMLMsgSmallSys += XMLText(wscMsg);
    wscXMLMsgSmallSys += L"</TEXT>";
    char szBufSmallSys[0xFFFF];
    uint iRetSmallSys;
    if (!HKHKSUCCESS(HkFMsgEncodeXML(wscXMLMsgSmallSys, szBufSmallSys, sizeof(szBufSmallSys), iRetSmallSys)))
        return;

    // send
    // for all players
    struct PlayerData* pPD = 0;
    while (pPD = Players.traverse_active(pPD))
    {
        uint iClientID = HkGetClientIdFromPD(pPD);
        uint iClientSystemID = 0;
        pub::Player::GetSystem(iClientID, iClientSystemID);

        char* szXMLBuf;
        int iXMLBufRet;
        char* szXMLBufSys;
        int iXMLBufRetSys;
        if (set_bUserCmdSetDieMsgSize && (ClientInfo[iClientID].dieMsgSize == CS_SMALL)) {
            szXMLBuf = szBufSmall;
            iXMLBufRet = iRetSmall;
            szXMLBufSys = szBufSmallSys;
            iXMLBufRetSys = iRetSmallSys;
        }
        else {
            szXMLBuf = szBuf;
            iXMLBufRet = iRet;
            szXMLBufSys = szBufSys;
            iXMLBufRetSys = iRetSys;
        }

        if (!set_bUserCmdSetDieMsg)
        { // /set diemsg disabled, thus send to all
            if (iSystemID == iClientSystemID)
                HkFMsgSendChat(iClientID, szXMLBufSys, iXMLBufRetSys);
            else
                HkFMsgSendChat(iClientID, szXMLBuf, iXMLBufRet);
            continue;
        }

        if (ClientInfo[iClientID].dieMsg == DIEMSG_NONE)
            continue;
        else if ((ClientInfo[iClientID].dieMsg == DIEMSG_SYSTEM) && (iSystemID == iClientSystemID))
            HkFMsgSendChat(iClientID, szXMLBufSys, iXMLBufRetSys);
        else if ((ClientInfo[iClientID].dieMsg == DIEMSG_SELF) && ((iClientID == iClientIDVictim) || (iClientID == iClientIDKiller)))
            HkFMsgSendChat(iClientID, szXMLBufSys, iXMLBufRetSys);
        else if (ClientInfo[iClientID].dieMsg == DIEMSG_ALL) {
            if (iSystemID == iClientSystemID)
                HkFMsgSendChat(iClientID, szXMLBufSys, iXMLBufRetSys);
            else
                HkFMsgSendChat(iClientID, szXMLBuf, iXMLBufRet);
        }
    }
}

void __stdcall ShipDestroyed(IObjRW* iobj, bool isKill, uint killerId)
{
    static uint lastShipId = 0;

    if (lastShipId == iobj->get_id())
    {
        return;
    }

    lastShipId = iobj->get_id();

    uint client = iobj->cobj->ownerPlayer;
    if (client)
    {
        HkIEngine::playerShips.erase(iobj->cobj->id);
        ClientInfo[client].iShipOld = ClientInfo[client].iShip;
        ClientInfo[client].iShip = 0;
    }

    TRY_HOOK{
        CALL_PLUGINS_V(PLUGIN_ShipDestroyed, __stdcall, (IObjRW * iobj, bool isKill, uint killerId), (iobj, isKill, killerId));

        if (isKill && client) { // a player was killed

            std::wstring wscEvent;
            wscEvent.reserve(256);
            wscEvent = L"kill";

            uint iSystemID;
            pub::Player::GetSystem(client, iSystemID);
            wchar_t wszSystem[64];
            swprintf(wszSystem, L"%u", iSystemID);

            DamageCause iCause = ClientInfo[client].dmgLastCause;
            uint iClientIDKiller = HkGetClientIDByShip(killerId);

            std::wstring wscVictim = (wchar_t*)Players.GetActiveCharacterName(client);
            wscEvent += L" victim=" + wscVictim;
            if (iClientIDKiller) {
                std::wstring wscType = L"";
                if (iCause == DamageCause::MissileTorpedo)
                    wscType = L"Missile/Torpedo";
                else if (iCause == DamageCause::Mine)
                    wscType = L"Mine";
                else if ((iCause == DamageCause::CruiseDisrupter) || (iCause == DamageCause::UnkDisrupter) || (iCause == DamageCause::DummyDisrupter))
                    wscType = L"Cruise Disruptor";
                else if (iCause == DamageCause::Collision)
                    wscType = L"Collision";
                else if (iCause == DamageCause::Gun)
                    wscType = L"Gun";
                else {
                    wscType = L"Gun"; //0x02
                    //				AddLog("get_cause() returned %X", iCause);
                }

                std::wstring wscMsg;
                if (client == iClientIDKiller) {
                    wscEvent += L" type=selfkill";
                    wscMsg = ReplaceStr(set_wscDeathMsgTextSelfKill, L"%victim", wscVictim);
                }
                else {
                    wscEvent += L" type=player";
                    std::wstring wscKiller = (wchar_t*)Players.GetActiveCharacterName(iClientIDKiller);
                    wscEvent += L" by=" + wscKiller;

                    wscMsg = ReplaceStr(set_wscDeathMsgTextPlayerKill, L"%victim", wscVictim);
                    wscMsg = ReplaceStr(wscMsg, L"%killer", wscKiller);
                }

                wscMsg = ReplaceStr(wscMsg, L"%type", wscType);
                if (set_bDieMsg && wscMsg.length())
                    SendDeathMsg(wscMsg, iSystemID, client, iClientIDKiller, iCause);
                ProcessEvent(L"%s", wscEvent.c_str());

            }
            else if (iCause == DamageCause::Admin) {
                std::wstring wscMsg = ReplaceStr(set_wscDeathMsgTextAdminKill, L"%victim", wscVictim);

                if (set_bDieMsg && wscMsg.length())
                    SendDeathMsg(wscMsg, iSystemID, client, 0, iCause);
            }
            else if (!killerId) {
                wscEvent += L" type=suicide";
                std::wstring wscMsg = ReplaceStr(set_wscDeathMsgTextSuicide, L"%victim", wscVictim);

                if (set_bDieMsg && wscMsg.length())
                    SendDeathMsg(wscMsg, iSystemID, client, 0, iCause);
                ProcessEvent(L"%s", wscEvent.c_str());
            }
            else
            {
                std::wstring wscType = L"";
                if (iCause == DamageCause::MissileTorpedo)
                    wscType = L"Missile/Torpedo";
                else if (iCause == DamageCause::Mine)
                    wscType = L"Mine";
                else if ((iCause == DamageCause::CruiseDisrupter) || (iCause == DamageCause::DummyDisrupter) || (iCause == DamageCause::UnkDisrupter))
                    wscType = L"Wasp/Hornet";
                else if (iCause == DamageCause::Collision)
                    wscType = L"Collision";
                else
                    wscType = L"Gun"; //0x02

                wscEvent += L" type=npc";
                std::wstring wscMsg = ReplaceStr(set_wscDeathMsgTextNPC, L"%victim", wscVictim);
                wscMsg = ReplaceStr(wscMsg, L"%type", wscType);

                if (set_bDieMsg && wscMsg.length())
                    SendDeathMsg(wscMsg, iSystemID, client, 0, iCause);
                ProcessEvent(L"%s", wscEvent.c_str());
            }
        }

    } CATCH_HOOK({})
}

FARPROC ShipDestroyedOrigFunc;

__declspec(naked) void ShipDestroyedNaked()
{
    __asm
    {
        push ecx
        push[esp + 0xC]
        push[esp + 0xC]
        push ecx
        call ShipDestroyed
        pop ecx
        mov eax, [ShipDestroyedOrigFunc]
        jmp eax
    }
}

void __stdcall SolarDestroyed(IObjRW* iobj, bool isKill, uint killerId)
{
    TRY_HOOK
    CALL_PLUGINS_V(PLUGIN_SolarDestroyed, __stdcall, (IObjRW * iobj, bool isKill, uint killerId), (iobj, isKill, killerId));
    CATCH_HOOK({})
}

FARPROC SolarDestroyedOrigFunc;

__declspec(naked) void SolarDestroyedNaked()
{
    __asm
    {
        push ecx
        push[esp + 0xC]
        push[esp + 0xC]
        push ecx
        call SolarDestroyed
        pop ecx
        mov eax, [SolarDestroyedOrigFunc]
        jmp eax
    }
}

inline int MineDestroyedPluginCaller(IObjRW* iobj, bool isKill, uint killerId)
{
    CALL_PLUGINS(PLUGIN_MineDestroyed, int, __stdcall, (IObjRW * iobj, bool isKill, uint killerId), (iobj, isKill, killerId));
    return 0;
}

bool __stdcall MineDestroyed(IObjRW* iobj, bool isKill, uint killerId)
{
    TRY_HOOK
        int pluginResult = MineDestroyedPluginCaller(iobj, isKill, killerId);
        switch (pluginResult)
        {
        case 0:
            return true;
        case 1:
            if (!isKill)
            {
                pub::SpaceObj::Destroy(((CSimple*)iobj->cobj)->id, DestroyType::EXPLODE);
                return false;
            }
            break;
        case 2:
            if (isKill)
            {
                pub::SpaceObj::Destroy(((CSimple*)iobj->cobj)->id, DestroyType::VANISH);
                return false;
            }
            break;
        }
        return true;
    CATCH_HOOK(AddLog("MineDestroyed exception"); return true)
}

FARPROC MineDestroyedOrigFunc;

__declspec(naked) void MineDestroyedNaked()
{
    __asm {
        push ecx
        push[esp + 0xC]
        push[esp + 0xC]
        push ecx
        call MineDestroyed
        pop ecx
        test al, al
        jz skipLabel
        mov eax, [MineDestroyedOrigFunc]
        jmp eax
        skipLabel :
        ret 0x8
    }
}

inline bool GuidedDestroyedPluginCaller(IObjRW* iobj, bool isKill, uint killerId)
{
    static IObjRW* lastIObj = nullptr;
    if (lastIObj == iobj)
    {
        return true;
    }
    lastIObj = iobj;

    CALL_PLUGINS(PLUGIN_GuidedDestroyed, bool, __stdcall, (IObjRW * iobj, bool isKill, uint killerId), (iobj, isKill, killerId));
    return true;
}

bool __stdcall GuidedDestroyed(IObjRW* iobj, bool isKill, uint killerId)
{
    TRY_HOOK
    if (!GuidedDestroyedPluginCaller(iobj, isKill, killerId))
    {
        pub::SpaceObj::Destroy(((CSimple*)iobj->cobj)->id, DestroyType::VANISH);
        return false;
    }
    return true;
    CATCH_HOOK(AddLog("GuidedDestroyed exception"); return true)
}

FARPROC GuidedDestroyedOrigFunc;

__declspec(naked) void GuidedDestroyedNaked()
{

    __asm {
        push ecx
        push[esp + 0xC]
        push[esp + 0xC]
        push ecx
        call GuidedDestroyed
        pop ecx
        test al, al
        jz skipLabel
        mov eax, [GuidedDestroyedOrigFunc]
        jmp eax
        skipLabel :
        ret 0x8
    }
}

void __stdcall LootDestroyed(IObjRW* iobj)
{
}

FARPROC LootDestroyedOrigFunc;

__declspec(naked) void LootDestroyedNaked()
{
    __asm {
        push ecx
        push ecx
        call LootDestroyed
        pop ecx
        jmp[LootDestroyedOrigFunc]
    }
}

FARPROC ShipAndSolarColGrpDeathOrigFunc;

void __stdcall ShipColGrpDestroyedHook(IObjRW* iobj, CArchGroup* colGrp, DamageEntry::SubObjFate fate, DamageList* dmgList)
{
    TRY_HOOK
    CALL_PLUGINS_V(PLUGIN_ShipColGrpDestroyed, , (IObjRW*, CArchGroup*, DamageEntry::SubObjFate fate, DamageList*), (iobj, colGrp, fate, dmgList));
    CATCH_HOOK({})
}

__declspec(naked) void ShipColGrpDestroyedHookNaked()
{
    __asm
    {
        push ecx
        push[esp + 0x10]
        push[esp + 0x10]
        push[esp + 0x10]
        push ecx
        call ShipColGrpDestroyedHook
        pop ecx
        mov eax, [ShipAndSolarColGrpDeathOrigFunc]
        jmp eax
    }
}

FARPROC ShipAndSolarEquipDeathOrigFunc;

void __stdcall ShipEquipDestroyedHook(IObjRW* iobj, CEquip* equip, DamageEntry::SubObjFate fate, DamageList* dmgList)
{
    TRY_HOOK
    CALL_PLUGINS_V(PLUGIN_ShipEquipDestroyed, __stdcall, (IObjRW*, CEquip*, DamageEntry::SubObjFate fate, DamageList*), (iobj, equip, fate, dmgList));
    CATCH_HOOK({})
}

__declspec(naked) void ShipEquipDestroyedHookNaked()
{
    __asm
    {
        push ecx
        push[esp + 0x10]
        push[esp + 0x10]
        push[esp + 0x10]
        push ecx
        call ShipEquipDestroyedHook
        pop ecx
        mov eax, [ShipAndSolarEquipDeathOrigFunc]
        jmp eax
    }
}

void __stdcall SolarColGrpDestroyedHook(IObjRW* iobj, CArchGroup* colGrp, DamageEntry::SubObjFate fate, DamageList* dmgList)
{
    TRY_HOOK
    CALL_PLUGINS_V(PLUGIN_SolarColGrpDestroyed, , (IObjRW*, CArchGroup*, DamageEntry::SubObjFate fate, DamageList*), (iobj, colGrp, fate, dmgList));
    CATCH_HOOK({})
}

__declspec(naked) void SolarColGrpDestroyedHookNaked()
{
    __asm
    {
        push ecx
        push[esp + 0x10]
        push[esp + 0x10]
        push[esp + 0x10]
        push ecx
        call SolarColGrpDestroyedHook
        pop ecx
        mov eax, [ShipAndSolarColGrpDeathOrigFunc]
        jmp eax
    }
}

void __stdcall SolarEquipDestroyedHook(IObjRW* iobj, CEquip* equip, DamageEntry::SubObjFate fate, DamageList* dmgList)
{
    TRY_HOOK
    CALL_PLUGINS_V(PLUGIN_SolarEquipDestroyed, __stdcall, (IObjRW*, CEquip*, DamageEntry::SubObjFate fate, DamageList*), (iobj, equip, fate, dmgList));
    CATCH_HOOK({})
}

__declspec(naked) void SolarEquipDestroyedHookNaked()
{
    __asm
    {
        push ecx
        push[esp + 0x10]
        push[esp + 0x10]
        push[esp + 0x10]
        push ecx
        call SolarEquipDestroyedHook
        pop ecx
        mov eax, [ShipAndSolarEquipDeathOrigFunc]
        jmp eax
    }
}