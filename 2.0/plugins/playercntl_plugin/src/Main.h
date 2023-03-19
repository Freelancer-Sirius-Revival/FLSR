// Player Control plugin for FLHookPlugin
// Feb 2010 by Cannon
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

#ifndef __MAIN_H__
#define __MAIN_H__ 1

#include <FLHook.h>

int extern set_iPluginDebug;
bool extern set_bEnablePimpShip;
bool extern set_bEnableRenameMe;
bool extern set_bEnableMoveChar;
bool extern set_bEnableMe;
bool extern set_bEnableDo;
bool extern set_bEnableWardrobe;
bool extern set_bEnableRestartCost;
bool extern set_bLocalTime;

// Imports from freelancer libraries.
namespace pub {
namespace SpaceObj {
IMPORT int __cdecl DrainShields(unsigned int);
IMPORT int __cdecl SetInvincible(unsigned int, bool, bool, float);
} // namespace SpaceObj

namespace Player {
IMPORT int GetRank(unsigned int const &iClientID, int &iRank);
}
} // namespace pub

// From EquipmentUtilities.cpp
namespace EquipmentUtilities {
void ReadIniNicknames();
const char *FindNickname(unsigned int hash);
} // namespace EquipmentUtilities

// From PurchaseRestrictions
namespace PurchaseRestrictions {
void LoadSettings(const std::string &scPluginCfgFile);
}

namespace Rename {
void LoadSettings(const std::string &scPluginCfgFile);
bool CreateNewCharacter(struct SCreateCharacterInfo const &si,
                         ClientId iClientID);
void CharacterSelect_AFTER(struct CHARACTER_ID const &charId,
                            ClientId iClientID);
bool UserCmd_RenameMe(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SetMoveCharCode(ClientId iClientID, const std::wstring &wscCmd,
                             const std::wstring &wscParam,
                             const wchar_t *usage);
bool UserCmd_MoveChar(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
void AdminCmd_SetAccMoveCode(CCmds *cmds, const std::wstring &wscCharname,
                             const std::wstring &wscCode);
void AdminCmd_ShowTags(CCmds *cmds);
void AdminCmd_AddTag(CCmds *cmds, const std::wstring &wscTag,
                     const std::wstring &wscPassword,
                     const std::wstring &description);
void AdminCmd_DropTag(CCmds *cmds, const std::wstring &wscTag);
bool UserCmd_DropTag(ClientId iClientID, const std::wstring &wscCmd,
                     const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_MakeTag(ClientId iClientID, const std::wstring &wscCmd,
                     const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SetTagPass(ClientId iClientID, const std::wstring &wscCmd,
                        const std::wstring &wscParam, const wchar_t *usage);
void Timer();
} // namespace Rename

namespace MiscCmds {
void LoadSettings(const std::string &scPluginCfgFile);
void ClearClientInfo(ClientId iClientID);
void BaseEnter(unsigned int iBaseID,  ClientId iClientID);
void CharacterInfoReq( ClientId iClientID, bool p2);
void Timer();
bool UserCmd_Pos(ClientId iClientID, const std::wstring &wscCmd,
                 const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Stuck(ClientId iClientID, const std::wstring &wscCmd,
                   const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Dice(ClientId iClientID, const std::wstring &wscCmd,
                  const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_DropRep(ClientId iClientID, const std::wstring &wscCmd,
                     const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Coin(ClientId iClientID, const std::wstring &wscCmd,
                  const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Lights(ClientId iClientID, const std::wstring &wscCmd,
                    const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SelfDestruct(ClientId iClientID, const std::wstring &wscCmd,
                          const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Shields(ClientId iClientID, const std::wstring &wscCmd,
                     const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Screenshot(ClientId iClientID, const std::wstring &wscCmd,
                        const std::wstring &wscParam, const wchar_t *usage);
void AdminCmd_SmiteAll(CCmds *cmds);
} // namespace MiscCmds

namespace IPBans {
void LoadSettings(const std::string &scPluginCfgFile);
void PlayerLaunch(unsigned int iShip,  ClientId iClientID);
void BaseEnter(unsigned int iBaseID,  ClientId iClientID);
void AdminCmd_ReloadBans(CCmds *cmds);
void AdminCmd_AuthenticateChar(CCmds *cmds, const std::wstring &wscCharname);
void ClearClientInfo(ClientId iClientID);
} // namespace IPBans

namespace PurchaseRestrictions {
void LoadSettings(const std::string &scPluginCfgFile);
void ClearClientInfo( ClientId iClientID);
void PlayerLaunch(unsigned int iShip,  ClientId iClientID);
void BaseEnter(unsigned int iBaseID,  ClientId iClientID);
bool GFGoodBuy(struct SGFGoodBuyInfo const &gbi,  ClientId iClientID);
bool ReqAddItem(unsigned int goodID, char const *hardpoint, int count,
                float status, bool mounted, ClientId iClientID);
bool ReqChangeCash(int iMoneyDiff,  ClientId iClientID);
bool ReqEquipment(class EquipDescList const &eqDesc,  ClientId iClientID);
bool ReqHullStatus(float fStatus,  ClientId iClientID);
bool ReqSetCash(int iMoney,  ClientId iClientID);
bool ReqShipArch(unsigned int iArchID,  ClientId iClientID);
} // namespace PurchaseRestrictions

namespace HyperJump {
void LoadSettings(const std::string &scPluginCfgFile);
void Timer();
bool SystemSwitchOutComplete(unsigned int iShip,  ClientId iClientID);
void SendDeathMsg(const std::wstring &wscMsg, uint iSystem,
                  uint iClientIDVictim, uint iClientIDKiller);
void ClearClientInfo( ClientId iClientID);
void PlayerLaunch(unsigned int iShip,  ClientId iClientID);
void MissileTorpHit(ClientId iClientID, DamageList *dmg);
void AdminCmd_Chase(CCmds *cmds, const std::wstring &wscCharname);
bool AdminCmd_Beam(CCmds *cmds, const std::wstring &wscCharname,
                   const std::wstring &wscTargetBaseName);
void AdminCmd_Pull(CCmds *cmds, const std::wstring &wscCharname);
void AdminCmd_Move(CCmds *cmds, float x, float y, float z);
void AdminCmd_TestBot(CCmds *cmds, const std::wstring &wscSystemNick,
                      int iCheckZoneTime);
void AdminCmd_JumpTest(CCmds *cmds, const std::wstring &fuse);

bool UserCmd_Survey(ClientId iClientID, const std::wstring &wscCmd,
                    const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SetCoords(ClientId iClientID, const std::wstring &wscCmd,
                       const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ChargeJumpDrive(ClientId iClientID, const std::wstring &wscCmd,
                             const std::wstring &wscParam,
                             const wchar_t *usage);
bool UserCmd_ActivateJumpDrive(ClientId iClientID, const std::wstring &wscCmd,
                               const std::wstring &wscParam,
                               const wchar_t *usage);
} // namespace HyperJump

namespace PimpShip {
void LoadSettings(const std::string &scPluginCfgFile);
void LocationEnter(unsigned int locationID,  ClientId iClientID);
bool UserCmd_PimpShip(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ShowSetup(ClientId iClientID, const std::wstring &wscCmd,
                       const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ShowItems(ClientId iClientID, const std::wstring &wscCmd,
                       const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ChangeItem(ClientId iClientID, const std::wstring &wscCmd,
                        const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_BuyNow(ClientId iClientID, const std::wstring &wscCmd,
                    const std::wstring &wscParam, const wchar_t *usage);
} // namespace PimpShip

namespace CargoDrop {
void LoadSettings(const std::string &scPluginCfgFile);
void Timer();
void SendDeathMsg(const std::wstring &wscMsg, uint iSystem,
                  uint iClientIDVictim, uint iClientIDKiller);
void ClearClientInfo( ClientId iClientID);
void SPObjUpdate(struct SSPObjUpdateInfo const &ui,  ClientId iClientID);
} // namespace CargoDrop

namespace Restart {
void LoadSettings(const std::string &scPluginCfgFile);
bool UserCmd_ShowRestarts(ClientId iClientID, const std::wstring &wscCmd,
                          const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Restart(ClientId iClientID, const std::wstring &wscCmd,
                     const std::wstring &wscParam, const wchar_t *usage);
void Timer();
} // namespace Restart

namespace RepFixer {
void LoadSettings(const std::string &scPluginCfgFile);
void PlayerLaunch(unsigned int iShip,  ClientId iClientID);
void BaseEnter(unsigned int iBaseID,  ClientId iClientID);
} // namespace RepFixer

namespace GiveCash {
void LoadSettings(const std::string &scPluginCfgFile);
void PlayerLaunch(unsigned int iShip,  ClientId iClientID);
void BaseEnter(unsigned int iBaseID,  ClientId iClientID);
bool UserCmd_GiveCash(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ShowCash(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SetCashCode(ClientId iClientID, const std::wstring &wscCmd,
                         const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_DrawCash(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
} // namespace GiveCash

namespace Message {
void LoadSettings(const std::string &scPluginCfgFile);
void ClearClientInfo( ClientId iClientID);
void Timer();
void DisConnect(ClientId iClientID, enum EFLConnection p2);
void CharacterInfoReq( ClientId iClientID, bool p2);
void PlayerLaunch(unsigned int iShip,  ClientId iClientID);
void BaseEnter(unsigned int iBaseID,  ClientId iClientID);
void SetTarget(uint uClientID, struct XSetTarget const &p2);
bool SubmitChat(CHAT_ID cId, unsigned long p1, const void *rdl, CHAT_ID cIdTo,
                int p2);
bool HkCb_SendChat(ClientId iClientID, uint iTo, uint iSize, void *pRDL);
bool UserCmd_SetMsg(ClientId iClientID, const std::wstring &wscCmd,
                    const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ShowMsgs(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SMsg(ClientId iClientID, const std::wstring &wscCmd,
                  const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_LMsg(ClientId iClientID, const std::wstring &wscCmd,
                  const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_GMsg(ClientId iClientID, const std::wstring &wscCmd,
                  const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ReplyToLastPMSender(ClientId iClientID, const std::wstring &wscCmd,
                                 const std::wstring &wscParam,
                                 const wchar_t *usage);
bool UserCmd_SendToLastTarget(ClientId iClientID, const std::wstring &wscCmd,
                              const std::wstring &wscParam,
                              const wchar_t *usage);
bool UserCmd_ShowLastPMSender(ClientId iClientID, const std::wstring &wscCmd,
                              const std::wstring &wscParam,
                              const wchar_t *usage);
bool UserCmd_PrivateMsg(ClientId iClientID, const std::wstring &wscCmd,
                        const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_PrivateMsgID(ClientId iClientID, const std::wstring &wscCmd,
                          const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_FactionMsg(ClientId iClientID, const std::wstring &wscCmd,
                        const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_FactionInvite(ClientId iClientID, const std::wstring &wscCmd,
                           const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SetChatTime(ClientId iClientID, const std::wstring &wscCmd,
                         const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Time(ClientId iClientID, const std::wstring &wscCmd,
                  const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_CustomHelp(ClientId iClientID, const std::wstring &wscCmd,
                        const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_BuiltInCmdHelp(ClientId iClientID, const std::wstring &wscCmd,
                            const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_MailShow(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_MailDel(ClientId iClientID, const std::wstring &wscCmd,
                     const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Me(ClientId iClientID, const std::wstring &wscCmd,
                const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Do(ClientId iClientID, const std::wstring &wscCmd,
                const std::wstring &wscParam, const wchar_t *usage);
bool RedText(std::wstring wscXMLMsg, uint iSystemID);
void UserCmd_Process(ClientId iClientID, const std::wstring &wscCmd);
void AdminCmd_SendMail(CCmds *cmds, const std::wstring &wscCharname,
                       const std::wstring &wscMsg);
void SendDeathMsg(const std::wstring &wscMsg, uint iSystem,
                  uint iClientIDVictim, uint iClientIDKiller);
} // namespace Message

namespace PlayerInfo {
bool UserCmd_ShowInfo(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_SetInfo(ClientId iClientID, const std::wstring &wscCmd,
                     const std::wstring &wscParam, const wchar_t *usage);
} // namespace PlayerInfo

namespace AntiJumpDisconnect {
void ClearClientInfo(ClientId iClientID);
void DisConnect( ClientId iClientID, enum EFLConnection state);
void JumpInComplete(unsigned int iSystem, unsigned int iShip,
                     ClientId iClientID);
void SystemSwitchOutComplete(unsigned int iShip,  ClientId iClientID);
void CharacterInfoReq( ClientId iClientID, bool p2);
} // namespace AntiJumpDisconnect

namespace SystemSensor {
void LoadSettings(const std::string &scPluginCfgFile);
bool UserCmd_ShowScan(ClientId iClientID, const std::wstring &wscCmd,
                      const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_Net(ClientId iClientID, const std::wstring &wscCmd,
                 const std::wstring &wscParam, const wchar_t *usage);
void ClearClientInfo(ClientId iClientID);
void PlayerLaunch(unsigned int iShip,  ClientId iClientID);
void JumpInComplete(unsigned int iSystem, unsigned int iShip,
                     ClientId iClientID);
void GoTradelane( ClientId iClientID, struct XGoTradelane const &xgt);
void StopTradelane( ClientId iClientID, unsigned int p1, unsigned int p2,
                   unsigned int p3);
void Dock_Call(unsigned int const &iShip, unsigned int const &iDockTarget,
               int iCancel, enum DOCK_HOST_RESPONSE response);
} // namespace SystemSensor

namespace Wardrobe {
void LoadSettings(const std::string &scPluginCfgFile);
bool UserCmd_ShowWardrobe(ClientId iClientID, const std::wstring &wscCmd,
                          const std::wstring &wscParam, const wchar_t *usage);
bool UserCmd_ChangeCostume(ClientId iClientID, const std::wstring &wscCmd,
                           const std::wstring &wscParam, const wchar_t *usage);
void Timer();
} // namespace Wardrobe

#endif
