// Death Penalty Plugin
// Ported from 88Flak by Raikkonen
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

// Includes
#include "Main.h"

// Load configuration file
void LoadSettings() {
    returncode = DEFAULT_RETURNCODE;

    // The path to the configuration file.
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    std::string configFile =
        std::string(szCurDir) + "\\flhook_plugins\\deathpenalty.cfg";

    INI_Reader ini;
    if (ini.open(configFile.c_str(), false)) {
        while (ini.read_header()) {
            // [General]
            if (ini.is_header("General"))
                while (ini.read_value()) {
                    if (ini.is_value("DeathPenaltyFraction"))
                        set_fDeathPenalty = ini.get_value_float(0);
                    else if (ini.is_value("DeathPenaltyKillerFraction"))
                        set_fDeathPenaltyKiller = ini.get_value_float(0);
                }

            // [ExcludedSystems]
            if (ini.is_header("ExcludedSystems"))
                while (ini.read_value()) {
                    if (ini.is_value("system"))
                        ExcludedSystems.push_back(
                            CreateID(ini.get_value_string(0)));
                }

            // [ShipOverrides]
            if (ini.is_header("ShipOverrides"))
                while (ini.read_value()) {
                    if (ini.is_value("ship"))
                        FractionOverridesbyShip[CreateID(
                            ini.get_value_string(0))] = ini.get_value_float(1);
                }
        }
        ini.close();
    }
}

//Nekura

    std::string StringBetween(std::string str, std::string first, std::string last)
    {
        int start;
        int end;

        start = str.find(first);
        end = str.find(last, start);
        std::string sreturn;

        if (start != std::string::npos && end != std::string::npos)
        {
            return sreturn = str.substr(start + first.length(), end - start - first.length());
        }
        else {
            return "";
        }
    }

    static bool startsWith(std::string_view str, std::string_view prefix) {
        return str.size() >= prefix.size() &&
               0 == str.compare(0, prefix.size(), prefix);
    }

    static bool endsWith(std::string_view str, std::string_view suffix) {
        return str.size() >= suffix.size() &&
               0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }

    std::vector<std::string> getHardpoints(std::string scParent, std::list<CMPDump_Entry> CMPList)
    {
        std::vector<std::string> vHardpoints;
        //Damaged CollGroup found
        for (std::list<CMPDump_Entry>::iterator CMPDump_iter = CMPList.begin(); CMPDump_iter != CMPList.end(); ++CMPDump_iter) {
            if (CMPDump_iter->bhasParent)
            {
                if (CMPDump_iter->scParent == scParent)
                {
                    //Add Hardpoint to List
                    if (!CMPDump_iter->bisCollGroup)
                    {
                        //ConPrint(L"Hardpoint: " + stows(CMPDump_iter->scData) + L" \n");
                        vHardpoints.push_back(CMPDump_iter->scData);
                    }
                    else {
                        //ConPrint(L"SubCollGrp: " + stows(CMPDump_iter->scData) + L" \n");
                        std::vector<std::string> vSubHardpoints = getHardpoints(CMPDump_iter->scData, CMPList);
                        //Combine vectors
                        vHardpoints.insert(vHardpoints.end(), vSubHardpoints.begin(), vSubHardpoints.end());
                    }
                }
            }

        }

        return vHardpoints;
    }

    // This will return a vector of CollisionGroups
    std::vector<std::string> HkGetCollisionGroups(uint iClientID, bool bOnly) {
        std::vector<std::string> vecString;
        std::vector<int> vecLine;
        std::wstring wscRet = L"";
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);

        wscRet = L"";
        std::wstring wscDir;
        if (!HKHKSUCCESS(HkGetAccountDirName(wscCharname, wscDir)))
            return vecString;

        std::wstring wscFile;
        HkGetCharFileName(wscCharname, wscFile);

        std::string scCharFile = scAcctPath + wstos(wscDir) + "\\" + wstos(wscFile) + ".fl";

        std::ifstream in(scCharFile);
        std::string str;
        int iCountLines = 1;
        // Read the next line from File untill it reaches the end.
        while (std::getline(in, str)) {
            // Line contains string of length > 0 then save it in vector
            if (str.size() > 0) {
                if (startsWith(str, "base_collision_group")) {
                    if (bOnly) {
                        if (!endsWith(str, "1")) {
                            vecString.push_back(str);
                            vecLine.push_back(iCountLines);
                        }
                    }
                    else {
                        vecString.push_back(str);
                        vecLine.push_back(iCountLines);
                    }
                }
            }
            iCountLines++;
        }
        in.close();
        return vecString;
    }
    
	// This will return a vector of Hardpoints from Damaged Collision Groups
    std::vector<std::string> GetHardpointsFromCollGroup(uint iClientID) {


        std::vector<std::string> vDamagedHardPoints;

        IObjRW* obj = HkGetInspect(iClientID);
        if (obj) {
            CShip* cship = (CShip*)(obj->cobj);
            if (cship) {
                std::string sShipfile = cship->shiparch()->szName;
                std::filesystem::path f(sShipfile);
                
                //FLPath
                char szCurDir[MAX_PATH];
                GetCurrentDirectory(sizeof(szCurDir), szCurDir);

                std::string cmpDump = std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string();
                cmpDump = cmpDump + ".cmp_dump";

                if (!std::filesystem::exists(cmpDump)) {
                    //CMPDump not found
                    ConPrint(L"ERROR: CMPDump not found for %s \n", stows(cship->shiparch()->szName).c_str());
                    ConPrint(L"ERROR: Dumpfilepath %s \n", stows(cmpDump).c_str());
                    return vDamagedHardPoints;
                }

                std::string CollisionGroupname;
                std::ifstream in(cmpDump);
                std::string str;
                bool damaged = false;
                int iCountLines = 1;
                int iCollGroupDeep = 0;
                int ilastCollGroupDeep = 0;
                std::string lastCollGroupData;
                //List of CMPDumpEntrys
                std::list<CMPDump_Entry> lCMPDump;

                //Parent Map
                std::map<int, ParentMap> mParent;


                //Get CollisionGroups and Hardpoints of CMP
                // Read the next line from File until it reaches the end.
                while (std::getline(in, str)) {
                    // Line contains string of length > 0
                    if (str.size() > 0) {
                        //Create new Entry
                        CMPDump_Entry entry;

                        //Count CollisionGroup Deep
                        size_t n = std::count(str.begin(), str.end(), '\t');
                        iCollGroupDeep = (int)n;

                        //Get last CollGroupDeep on Deepchange
                        if (iCollGroupDeep != ilastCollGroupDeep) {
                            //Update Parent Map

                            //On Deep ++
                            if (iCollGroupDeep > ilastCollGroupDeep)
                            {
                                if (mParent[iCollGroupDeep].scFirstParent == "")
                                    mParent[iCollGroupDeep].scFirstParent = lastCollGroupData;

                                mParent[iCollGroupDeep].scParent = lastCollGroupData;
                            }

                            //On Deep --
                            if (iCollGroupDeep < ilastCollGroupDeep)
                            {
                                mParent[iCollGroupDeep + 1].scParent = "";
                            }


                            ilastCollGroupDeep = iCollGroupDeep;

                        }

                        //Check if found Entry is a CollisionGroup or a Hardpoint
                        if (str.find("> ") != std::string::npos) {
                            entry.bisCollGroup = true;

                        }
                        else {
                            entry.bisCollGroup = false;
                        }

                        //Check if CollGroup or Hardpoint has a Parent
                        if (iCollGroupDeep > 0) {
                            entry.bhasParent = true;

                            if (entry.bisCollGroup)
                            {
                                entry.scParent = mParent[iCollGroupDeep].scFirstParent;
                            }
                            else {
                                entry.scParent = mParent[iCollGroupDeep].scParent;

                            }

                        }
                        else
                        {
                            entry.scParent = mParent[iCollGroupDeep].scParent;
                            entry.bhasParent = false;
                        }

                        //Get Data of CollisionGroup or Hardpoint
                        //CollGrp
                        if (entry.bisCollGroup)
                        {
                            entry.scData = str.substr(str.find("> ") + 2);
                            lastCollGroupData = entry.scData;
                        }
                        else
                        {
                            entry.scData = StringBetween(str, "[", ":");
                        }

                        lCMPDump.push_back(entry);

                    }
                    iCountLines++;
                }
                in.close();


                //Check which Hardpoints are damaged
                std::vector<std::string> vDamagedCollGrps = HkGetCollisionGroups(iClientID, true);
                for (std::vector<std::string>::iterator it = vDamagedCollGrps.begin(); it != vDamagedCollGrps.end(); ++it) {

                    //Get Data
                    std::string scCollGroup_Charfile = *it;
                    int pos = scCollGroup_Charfile.find("=");
                    scCollGroup_Charfile = scCollGroup_Charfile.substr(pos + 1, scCollGroup_Charfile.length());
                    std::string scCollGroup = StringBetween(scCollGroup_Charfile, " ", ",");

                    //Check for Hardpoints on CollGrp
                    for (std::list<CMPDump_Entry>::iterator CMPDump_iter = lCMPDump.begin(); CMPDump_iter != lCMPDump.end(); ++CMPDump_iter) {
                        if (CMPDump_iter->scParent == scCollGroup)
                        {
                            //Add Hardpoint to List
                            if (!CMPDump_iter->bisCollGroup)
                            {
                                vDamagedHardPoints.push_back(CMPDump_iter->scData);
                            }
                            else
                            {
                                //Found SubCollGroup
                                std::vector<std::string> vSubHardpoints = getHardpoints(CMPDump_iter->scData, lCMPDump);

                                //Combine Vectors
                                vDamagedHardPoints.insert(vDamagedHardPoints.end(), vSubHardpoints.begin(), vSubHardpoints.end());

                            }

                        }

                    }
                }

            }
        }

        return vDamagedHardPoints;

    }


    float CaclDestroyedHardpointWorth(uint iClientID) {
        std::vector<std::string> DamagedHardPoints = GetHardpointsFromCollGroup(iClientID);

        // Player cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

        // Add mounted Equip to list
        std::list<CARGO_INFO> lstMounted;
        float fValue = 0;
        for (auto& cargo : lstCargo) {
            if (!cargo.bMounted)
                continue;

            // Check Archtype
            Archetype::Equipment* eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();
            if (aType == Archetype::SHIELD_GENERATOR ||
                aType == Archetype::THRUSTER || aType == Archetype::LAUNCHER ||
                aType == Archetype::GUN || aType == Archetype::MINE_DROPPER ||
                aType == Archetype::COUNTER_MEASURE_DROPPER || aType == Archetype::CLOAKING_DEVICE) {

                for (std::vector<std::string>::iterator it = DamagedHardPoints.begin(); it != DamagedHardPoints.end(); ++it) {
                    if (startsWith(*it, cargo.hardpoint.value)) {

                        const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                        if (gi) {
                            gi = GoodList::find_by_id(gi->iArchID);
                            if (gi) {
                                // float *fResaleFactor = (float *)((char
                                // *)hModServer + 0x8AE78);
                                float fItemValue = gi->fPrice; //* (*fResaleFactor);
                                fValue += fItemValue;
                            }
                        }
                    }
                }
            }
        }
        return fValue;
    }
//Nekura END

void ClearClientInfo(uint iClientID) {
    returncode = DEFAULT_RETURNCODE;
    MapClients.erase(iClientID);
}

// Is the player is a system that is excluded from death penalty?
bool bExcludedSystem(uint iClientID) {
    // Get System ID
    uint iSystemID;
    pub::Player::GetSystem(iClientID, iSystemID);
    // Search list for system
    return (std::find(ExcludedSystems.begin(), ExcludedSystems.end(),
                      iSystemID) != ExcludedSystems.end());
}

// This returns the override for the specific ship as defined in the cfg file.
// If there is not override it returns the default value defined as
// "DeathPenaltyFraction" in the cfg file
float fShipFractionOverride(uint iClientID) {
    // Get ShipArchID
    uint iShipArchID;
    pub::Player::GetShipID(iClientID, iShipArchID);

    // Default return value is the default death penalty fraction
    float fOverrideValue = set_fDeathPenalty;

    // See if the ship has an override fraction
    if (FractionOverridesbyShip.find(iShipArchID) !=
        FractionOverridesbyShip.end())
        fOverrideValue = FractionOverridesbyShip[iShipArchID];

    return fOverrideValue;
}

// Hook on Player Launch. Used to work out the death penalty and display a
// message to the player warning them of such
void __stdcall PlayerLaunch_AFTER(unsigned int iShip, unsigned int iClientID) {
    returncode = DEFAULT_RETURNCODE;


    MapClients[iClientID].DeathPenaltyCredits = 0;
    // No point in processing anything if there is no death penalty
    if (set_fDeathPenalty) {

        // Check to see if the player is in a system that doesn't have death
        // penalty
        if (!bExcludedSystem(iClientID)) {

            // Get the players net worth
            float fValue;
            pub::Player::GetAssetValue(iClientID, fValue);

            //NEKURA - FLSR Adjust Calc | START
            float iWorth = CaclDestroyedHardpointWorth(iClientID);
            fValue -= iWorth;
            // NEKURA - FLSR Adjust Calc | END

            // Calculate what the death penalty would be upon death
            const int finalValue = static_cast<int>(fValue * fShipFractionOverride(set_fDeathPenalty));
            if (finalValue < 0)
                ConPrint(L"Death penalty is negative! " + std::to_wstring(finalValue));
            else
                MapClients[iClientID].DeathPenaltyCredits = finalValue;

            // Should we print a death penalty notice?
            if (MapClients[iClientID].bDisplayDPOnLaunch)
                PrintUserCmdText(
                    iClientID,
                    L"Notice: the death penalty for your ship will be " +
                        ToMoneyStr(MapClients[iClientID].DeathPenaltyCredits) +
                        L" credits.  Type /dp for more information.");
        }
    }
}

void LoadUserCharSettings(uint iClientID) {
    returncode = DEFAULT_RETURNCODE;

    // Get Account directory then flhookuser.ini file
    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    std::wstring wscDir;
    HkGetAccountDirName(acc, wscDir);
    std::string scUserFile = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";

    // Get char filename and save setting to flhookuser.ini
    std::wstring wscFilename;
    HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
    std::string scFilename = wstos(wscFilename);
    std::string scSection = "general_" + scFilename;

    // read death penalty settings
    CLIENT_DATA c;
    c.bDisplayDPOnLaunch = IniGetB(scUserFile, scSection, "DPnotice", true);
    MapClients[iClientID] = c;
}



// Function that will apply the death penalty on a player death
void PenalizeDeath(uint iClientID, uint iKillerID) {
    if (!set_fDeathPenalty)
        return;

    //Nekura
    //Check for FightInfo
    std::wstring wscCharFilename;
    HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);

    std::wstring wscKillerCharFilename;
    HkGetCharFileName(ARG_CLIENTID(iKillerID), wscKillerCharFilename);

    // Valid iClientID and the ShipArch or System isnt in the excluded list?
    if (iClientID != -1 && !bExcludedSystem(iClientID)) {

        // Get the players cash
        int iCash;
        HkGetCash(ARG_CLIENTID(iClientID), iCash);

        // Get how much the player owes
        int iOwed = MapClients[iClientID].DeathPenaltyCredits;

        // If the amount the player owes is more than they have, set the
        // amount to their total cash
        if (iOwed > iCash)
            iOwed = iCash;

        // If another player has killed the player
		if (iKillerID && set_fDeathPenaltyKiller && iKillerID != iClientID) {
            int iGive = (int)(iOwed * set_fDeathPenaltyKiller);
            if (iGive) {
                // Reward the killer, print message to them
                HkAddCash(ARG_CLIENTID(iKillerID), iGive);
                PrintUserCmdText(iKillerID,
                                 L"Death penalty: given " + ToMoneyStr(iGive) +
                                     L" credits from %s's death penalty.",
                                 Players.GetActiveCharacterName(iClientID));
            }
        }

        if (iOwed) {
            // Print message to the player and remove cash
            PrintUserCmdText(iClientID, L"Death penalty: charged " +
                                            ToMoneyStr(iOwed) + L" credits.");
            HkAddCash(ARG_CLIENTID(iClientID), -iOwed);
        }
    }
}

void __stdcall ShipDestroyed(IObjRW* iobj, bool isKill, uint killerId)
{
    returncode = DEFAULT_RETURNCODE;

    if (!isKill)
        return;

    uint victimClientId = iobj->cobj->GetOwnerPlayer();
    if (victimClientId)
    {
        uint killerClientId = HkGetClientIDByShip(killerId);
        PenalizeDeath(victimClientId, victimClientId != killerClientId ? killerClientId : 0);
    }
}

// This will save whether the player wants to receieve the /dp notice or not to
// the flhookuser.ini file
void SaveDPNoticeToCharFile(uint iClientID, std::string value) {
    std::wstring wscDir, wscFilename;
    CAccount *acc = Players.FindAccountFromClientID(iClientID);
    if (HKHKSUCCESS(HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename)) &&
        HKHKSUCCESS(HkGetAccountDirName(acc, wscDir))) {
        std::string scUserFile =
            scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
        std::string scSection = "general_" + wstos(wscFilename);
        IniWrite(scUserFile, scSection, "DPnotice", value);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USER COMMANDS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// /dp command. Shows information about death penalty
bool UserCmd_DP(uint iClientID, const std::wstring &wscCmd,
                const std::wstring &wscParam, const wchar_t *usage) {

    // If there is no death penalty, no point in having death penalty commands
    if (!set_fDeathPenalty) {
        return true;
    }

    if (wscParam.length()) // Arguments passed
    {
        if (ToLower(Trim(wscParam)) == L"off") {
            MapClients[iClientID].bDisplayDPOnLaunch = false;
            SaveDPNoticeToCharFile(iClientID, "no");
            PrintUserCmdText(iClientID, L"Death penalty notices disabled.");
        } else if (ToLower(Trim(wscParam)) == L"on") {
            MapClients[iClientID].bDisplayDPOnLaunch = true;
            SaveDPNoticeToCharFile(iClientID, "yes");
            PrintUserCmdText(iClientID, L"Death penalty notices enabled.");
        } else {
            PrintUserCmdText(iClientID, L"ERR Invalid parameters");
            PrintUserCmdText(iClientID, usage);
        }
    } else {
        PrintUserCmdText(
            iClientID,
            L"The death penalty is charged immediately when you die.");
        if (!bExcludedSystem(iClientID)) {
            int iOwed = MapClients[iClientID].DeathPenaltyCredits;
            if (iOwed > 0) {
                PrintUserCmdText(iClientID,
                                 L"The death penalty for your ship will be " +
                                     ToMoneyStr(iOwed) + L" credits.");
                PrintUserCmdText(iClientID,
                                 L"If you would like to turn off the death "
                                 L"penalty notices, run "
                                 L"this command with the argument \"off\".");
            } else {
                PrintUserCmdText(iClientID, L"You don't have to pay the death penalty");
            }
        } else {
            PrintUserCmdText(iClientID,
                             L"You don't have to pay the death penalty "
                             L"because you are in a specific system.");
        }
    }
    return true;
}

// Additional information related to the plugin when the /help command is used
void UserCmd_Help(uint iClientID, const std::wstring &wscParam) {
    returncode = DEFAULT_RETURNCODE;
    PrintUserCmdText(iClientID, L"/dp");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USER COMMAND PROCESSING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Define usable chat commands here
USERCMD UserCmds[] = {
    {L"/dp", UserCmd_DP, L"Usage: /dp"},
};

// Process user input
bool UserCmd_Process(uint iClientID, const std::wstring &wscCmd) {
    returncode = DEFAULT_RETURNCODE;

    try {
        std::wstring wscCmdLineLower = ToLower(wscCmd);

        // If the chat std::string does not match the USER_CMD then we do not
        // handle the command, so let other plugins or FLHook kick in. We
        // require an exact match
        for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++) {
            if (wscCmdLineLower.find(UserCmds[i].wszCmd) == 0) {
                // Extract the parameters std::string from the chat std::string.
                // It should be immediately after the command and a space.
                std::wstring wscParam = L"";
                if (wscCmd.length() > wcslen(UserCmds[i].wszCmd)) {
                    if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
                        continue;
                    wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
                }

                // Dispatch the command to the appropriate processing function.
                if (UserCmds[i].proc(iClientID, wscCmd, wscParam,
                                     UserCmds[i].usage)) {
                    // We handled the command tell FL hook to stop processing
                    // this chat std::string.
                    returncode =
                        SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command,
                                                    // return immediatly
                    return true;
                }
            }
        }
    } catch (...) {
        AddLog("ERROR: Exception in UserCmd_Process(iClientID=%u, wscCmd=%s)",
               iClientID, wstos(wscCmd).c_str());
        LOG_EXCEPTION;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Do things when the dll is loaded
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    srand((uint)time(0));

    // If we're being loaded from the command line while FLHook is running then
    // set_scCfgFile will not be empty so load the settings as FLHook only
    // calls load settings on FLHook startup and .rehash.
    if (fdwReason == DLL_PROCESS_ATTACH && set_scCfgFile.length() > 0)
        LoadSettings();

    return true;
}

// Functions to hook
EXPORT PLUGIN_INFO *Get_PluginInfo() {
    PLUGIN_INFO *p_PI = new PLUGIN_INFO();
    p_PI->sName = "Death Penalty";
    p_PI->sShortName = "deathpenalty";
    p_PI->bMayPause = true;
    p_PI->bMayUnload = true;
    p_PI->ePluginReturnCode = &returncode;
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&LoadSettings, PLUGIN_LoadSettings, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&UserCmd_Process,
                                             PLUGIN_UserCmd_Process, 0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&UserCmd_Help, PLUGIN_UserCmd_Help, 0));
    p_PI->lstHooks.push_back(
        PLUGIN_HOOKINFO((FARPROC *)&ShipDestroyed, PLUGIN_ShipDestroyed, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&PlayerLaunch_AFTER,
                        PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&LoadUserCharSettings,
                                             PLUGIN_LoadUserCharSettings, 0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC *)&ClearClientInfo,
                                             PLUGIN_ClearClientInfo, 0));
    return p_PI;
}
