#include "main.h"

namespace AntiCheat {

     AC_INFO AC_Info[MAX_CLIENT_ID + 1];
    
     
     // Timing CheatDetection
    namespace TimingAC {
        void CheckTimeStamp(struct SSPObjUpdateInfo const &pObjInfo,  ClientId iClientID) {

            if (AC_Info[iClientID].dServerTimestamp == 0.0) 
            {
                AC_Info[iClientID].dServerTimestamp = (double)GetTimeInMS() / 1000.0;
                AC_Info[iClientID].dClientTimestamp = (double)pObjInfo.fTimestamp; // Its a double!!
                AC_Info[iClientID].iRateDetectCountTimingSpeed = 0;
            } else {
                //Timstamp Calc
                double dServerTime = ((double)GetTimeInMS() / 1000.0) - AC_Info[iClientID].dServerTimestamp;
                double dClientTime = (double)pObjInfo.fTimestamp - AC_Info[iClientID].dClientTimestamp;
                double dRate = dClientTime / dServerTime;

                if ((dRate > 1.1)) {
                    AC_Info[iClientID].iRateDetectCountTimingSpeed++;
                }
            
                //If Detect > 10 in 10 Sekunden
                if (AC_Info[iClientID].iRateDetectCountTimingSpeed > 20 && !AC_Info[iClientID].bTimingSpeedCheater) {
                    AntiCheat::Reporting::ReportCheater(iClientID, "TimingSpeed", "RATE: "+std::to_string(dRate));
                    AC_Info[iClientID].bTimingSpeedCheater = true;
                }


                // Reset
                if (dServerTime > 10) {
                    AC_Info[iClientID].dServerTimestamp = 0.0;
                    AC_Info[iClientID].dClientTimestamp = 0.0;
                    AC_Info[iClientID].iRateDetectCountTimingSpeed = 0;
                }


            }
        }

        void Init( ClientId iClientID)
        {
            AC_Info[iClientID].dServerTimestamp = 0.0;
            AC_Info[iClientID].dClientTimestamp = 0.0;
            AC_Info[iClientID].iRateDetectCountTimingSpeed = 0;
        }

    }

    // Speed CheatDetecion
    namespace SpeedAC {

        void Init(ClientId iClientID)
        {
            AC_Info[iClientID].tmSpeedExceptionTimeout = GetTimeInMS() + 60000;
            AC_Info[iClientID].fLastSpeedTimestamp = 0.0f;
            AC_Info[iClientID].engineState = HkGetEngineState(iClientID);
            AC_Info[iClientID].fAllowedCruiseSpeed = 0;
            AC_Info[iClientID].fAllowedThrusterSpeed = 0;
            AC_Info[iClientID].fAllowedEngineSpeed = 0;
            AC_Info[iClientID].fAllowedTradelaneSpeed = 0;
            AC_Info[iClientID].fAllowedStrafeSpeed = 0;
            AC_Info[iClientID].vecDistances.clear();
            AC_Info[iClientID].vecTimes.clear();
            AC_Info[iClientID].tmCheckTime = 0;
            AC_Info[iClientID].iSpeedDetections = 0;
        }

        void UpdateShipSpeed(ClientId iClientID)
        {

	        IObjInspectImpl *obj = HkGetInspect(iClientID);
	        if (obj)
	        {
                CShip *cship = (CShip *)HkGetEqObjFromObjRW((IObjRW *)obj);

		        float fMaxForce = 0.0f;
		        float fMaxThrusterForce = 0.0f;
		        float fDrag = cship->shiparch()->fLinearDrag;
            
                CEquipTraverser tr;
                for (CEquip *Equip = GetEquipManager(cship)->Traverse(tr); Equip;
                     Equip = GetEquipManager(cship)->Traverse(tr)) {

			        if (CEThruster::cast(Equip))
			        {
				        CEThruster *cethruster = (CEThruster*)Equip;
				        fMaxThrusterForce += cethruster->ThrusterArch()->fMaxForce;
			        }
			        else if (CEEngine::cast(Equip))
			        {
				        CEEngine *ceengine = (CEEngine*)Equip;
				        fDrag += ceengine->EngineArch()->fLinearDrag;
				        fMaxForce += ceengine->EngineArch()->fMaxForce;
			        }
		        }

		        AC_Info[iClientID].fAllowedStrafeSpeed = cship->shiparch()->fStrafeForce / fDrag;
		        AC_Info[iClientID].fAllowedEngineSpeed = sqrt(powf(fMaxForce / fDrag, 2) + powf(AC_Info[iClientID].fAllowedStrafeSpeed, 2));
		        AC_Info[iClientID].fAllowedThrusterSpeed = sqrt(powf((fMaxForce + fMaxThrusterForce)  / fDrag, 2) + powf(AC_Info[iClientID].fAllowedStrafeSpeed, 2));

		        float fMaxCruiseSpeed = 0.0f;
		        #define ADDR_COMMON_CRUISE_SPEED 0x18B5CC
		        ReadProcMem((char*)hModCommon+ADDR_COMMON_CRUISE_SPEED, &fMaxCruiseSpeed, sizeof(fMaxCruiseSpeed));
		        AC_Info[iClientID].fAllowedCruiseSpeed = sqrt(powf(fMaxCruiseSpeed, 2) + powf(AC_Info[iClientID].fAllowedStrafeSpeed, 2));

		        #define ADDR_COMMON_MAX_TRADELANE_SPEED 0x13F3CC 
		        ReadProcMem((char*)hModCommon+ADDR_COMMON_MAX_TRADELANE_SPEED, &AC_Info[iClientID].fAllowedTradelaneSpeed , sizeof(AC_Info[iClientID].fAllowedTradelaneSpeed ));
		
	        }
        }

        float GetPlayerAllowedSpeed(ClientId iClientID, enum ENGINE_STATE state) {
            switch (state) {
            case ES_CRUISE:
                return AC_Info[iClientID].fAllowedCruiseSpeed;      
            case ES_THRUSTER:
                return AC_Info[iClientID].fAllowedThrusterSpeed;
            case ES_ENGINE:
                return AC_Info[iClientID].fAllowedEngineSpeed;
            case ES_TRADELANE:
                return AC_Info[iClientID].fAllowedTradelaneSpeed;
            default:
                return 0.0f;
            }
        }
    
        bool CheckClientSpeed(ClientId iClientID, std::vector<float>& vecTimes, std::vector<float>& vecDistances, enum ENGINE_STATE engineState)
        {
            float fMaxSpeed = GetPlayerAllowedSpeed(iClientID, engineState);

            if (vecDistances.size() < 20 || fMaxSpeed == 0)
                return false;

            float fDistance = accumulate(vecDistances.begin(), vecDistances.end(), 0.0f);
            float fTime = accumulate(vecTimes.begin(), vecTimes.end(), 0.0f);
            float fAvSpeed = fDistance / fTime;

			if (fAvSpeed > (fMaxSpeed + 175.0f) && !AC_Info[iClientID].bSpeedCheater)
			{
                AC_Info[iClientID].iSpeedDetections++;

                std::string msg = "speed=" + std::to_string(fAvSpeed) + " allowed=" + std::to_string(fMaxSpeed);
                msg += " samples=";
                for (size_t i = 0; i < vecDistances.size(); i++)
                    msg += std::to_string(vecDistances[i] / vecTimes[i]) + "<br>";

                if (AC_Info[iClientID].iSpeedDetections > 2 && engineState != ES_ENGINE)
                {
					AntiCheat::Reporting::ReportCheater(iClientID, "Speed", msg);
                    AC_Info[iClientID].iSpeedDetections = 0;
                    AC_Info[iClientID].bSpeedCheater = true;
                }
                else if (AC_Info[iClientID].iSpeedDetections > 3 && engineState == ES_ENGINE)
                {
                    AntiCheat::Reporting::ReportCheater(iClientID, "Speed normal-engine", msg);
                    AC_Info[iClientID].iSpeedDetections = 0;
                    AC_Info[iClientID].bSpeedCheater = true;
                }

                AC_Info[iClientID].vecDistances.clear();
                AC_Info[iClientID].vecTimes.clear();
                return true;
            }

            return false;
        }

        // vDunno
        void vDunno1(ClientId iClientID, mstime delay)
        {
            mstime tmNewSpeedExceptionTimeout = GetTimeInMS() + delay;
            if (AC_Info[iClientID].tmSpeedExceptionTimeout < tmNewSpeedExceptionTimeout)
                
                AC_Info[iClientID].tmSpeedExceptionTimeout = tmNewSpeedExceptionTimeout;
                AC_Info[iClientID].vecDistances.clear();
                AC_Info[iClientID].vecDistances.clear();
        }

        // vDunno
        void vDunno2(ClientId iClientID)
        {
            AC_Info[iClientID].tmSpeedExceptionTimeout = 0;
        }

        int iDunno3(unsigned int const& iShip, unsigned int const& iDockTarget, int iCancel, enum DOCK_HOST_RESPONSE response)
        {
            ClientId iClientID = HkGetClientIDByShip(iShip);
            if (iClientID)
            {
                uint iTypeID;
                pub::SpaceObj::GetType(iDockTarget, iTypeID);
                if (iTypeID == 0x40 || iTypeID == 0x800)
                {
                    if (!iCancel)
                    {
                        vDunno1(iClientID, 20000);
                    }
                    else
                    {
                        vDunno2(iClientID);
                    }
                }
            }
            return 0;
        }

		

        void CheckSpeedCheat(struct SSPObjUpdateInfo const& pObjInfo,  ClientId iClientID)
        {
            mstime now = GetTimeInMS();

            if (AC_Info[iClientID].fAllowedEngineSpeed == 0)
                UpdateShipSpeed(iClientID);

            enum ENGINE_STATE engineState = HkGetEngineState(iClientID);
            if (engineState == ES_KILLED)
                engineState = AC_Info[iClientID].engineState;

            IObjInspectImpl* formation_leader = 0;
            IObjInspectImpl* obj = HkGetInspect(iClientID);
            if (obj)
            {
                obj->get_formation_leader((IObjRW*&)formation_leader);
                if (formation_leader != NULL)
                {
                    uint iLeaderClientID = HkGetClientIDByShip(formation_leader->get_id());
                    if (HkIsValidClientID(iLeaderClientID))
                    {

                        if (AC_Info[iLeaderClientID].tmSpeedExceptionTimeout > AC_Info[iClientID].tmSpeedExceptionTimeout)
                        {
                            AC_Info[iClientID].tmSpeedExceptionTimeout = AC_Info[iLeaderClientID].tmSpeedExceptionTimeout;
                        }
                    }
                }
            }

            //Dunno
            if (GetPlayerAllowedSpeed(iClientID, engineState) < GetPlayerAllowedSpeed(iClientID, AC_Info[iClientID].engineState))
            {
                vDunno1(iClientID, 3000);
            }


            else if (GetPlayerAllowedSpeed(iClientID, engineState) > GetPlayerAllowedSpeed(iClientID, AC_Info[iClientID].engineState))
            {
                vDunno2(iClientID);
            }

            else if (AC_Info[iClientID].fLastSpeedTimestamp && now > AC_Info[iClientID].tmSpeedExceptionTimeout)
            {
                float fDeltaSecs = (float)(pObjInfo.fTimestamp - AC_Info[iClientID].fLastSpeedTimestamp);
                float fDistance = sqrt(powf((AC_Info[iClientID].vLastPos.x - pObjInfo.vPos.x), 2)
                    + powf((AC_Info[iClientID].vLastPos.y - pObjInfo.vPos.y), 2)
                    + powf((AC_Info[iClientID].vLastPos.z - pObjInfo.vPos.z), 2));

                if (formation_leader)
                    fDistance *= (300.0f / 360.0f);

                std::vector<float>& vecDistances = AC_Info[iClientID].vecDistances[engineState];
                if (vecDistances.size() > 20)
                    vecDistances.erase(vecDistances.begin());
                
                vecDistances.push_back(fDistance);

                std::vector<float>& vecTimes = AC_Info[iClientID].vecTimes[engineState];
                if (vecTimes.size() > 20)
                    vecTimes.erase(vecTimes.begin());
                
                vecTimes.push_back(fDeltaSecs);

                if (AC_Info[iClientID].tmCheckTime < now)
                {
                    AC_Info[iClientID].tmCheckTime = now + 1000;
                    CheckClientSpeed(iClientID, vecTimes, vecDistances, engineState);
                }

                // Check for save position cheat. If the ship moves by more than 5km between position 
                // updates then it's almost certainly cheating.
                //if (fDistance > 10000.0f)
                //{
                    // LogCheater(iClientID, L"poshack, jump=" + ftows((float)fDistance));
                    // FIXME: Kill the ship.
                //}
            }

            AC_Info[iClientID].fLastSpeedTimestamp = pObjInfo.fTimestamp;
            AC_Info[iClientID].vLastPos = pObjInfo.vPos;
            AC_Info[iClientID].engineState = engineState;
        }


		
    }

    // Power CheatDetection
    namespace PowerAC {
		
        void Init( ClientId iClientID)
        {
            AC_Info[iClientID].bSetupPowerCheatDet = false;
        }

        void Setup( ClientId iClientID)
        {
            AC_Info[iClientID].bSetupPowerCheatDet = true;
            AC_Info[iClientID].fMaxCapacity = 0.0f;
            AC_Info[iClientID].fChargeRate = 0.0f;
            AC_Info[iClientID].fCurEstCapacity = 0.0f;

            IObjInspectImpl* obj = HkGetInspect(iClientID);
            if (obj)
            {
                CShip* cship = (CShip*)HkGetEqObjFromObjRW((IObjRW*)obj);

                // Find the ship power plant and record the capacity and recharge rates

                CEquipTraverser tr;
                for (CEquip* Equip = GetEquipManager(cship)->Traverse(tr); Equip;
                    Equip = GetEquipManager(cship)->Traverse(tr)) {
                    if (CEPower::cast(Equip))
                    {
                        CEPower* cepower = (CEPower*)Equip;
                        AC_Info[iClientID].fMaxCapacity = cepower->GetCapacity();
                        AC_Info[iClientID].fChargeRate = cepower->GetChargeRate();
                    }
          
                }

 
                AC_Info[iClientID].fCurEstCapacity = AC_Info[iClientID].fMaxCapacity;
            }

            AC_Info[iClientID].tmLastPowerUpdate = GetTimeInMS();
            AC_Info[iClientID].fMinCapacity = 0.0f - (AC_Info[iClientID].fChargeRate * 5);
            AC_Info[iClientID].fMaxCapacity = AC_Info[iClientID].fMaxCapacity + (AC_Info[iClientID].fChargeRate * 2);

        }		

        void FireWeapon( ClientId iClientID, struct XFireWeaponInfo const wpn)
        {
            if (!AC_Info[iClientID].bSetupPowerCheatDet)
            {
                Setup(iClientID);
            }

            // Estimate power charge since last call here.
            mstime msTimeNow = GetTimeInMS();
            float fTimeDelta = ((float)(msTimeNow - AC_Info[iClientID].tmLastPowerUpdate)) / 1000.0f;
            AC_Info[iClientID].tmLastPowerUpdate = msTimeNow;
            AC_Info[iClientID].fCurEstCapacity += fTimeDelta * AC_Info[iClientID].fChargeRate;

            IObjInspectImpl* obj = HkGetInspect(iClientID);
            if (obj)
            {
                CShip* cship = (CShip*)HkGetEqObjFromObjRW((IObjRW*)obj);
                

                for (ushort* slot = wpn.sHpIdsBegin; slot != wpn.sHpIdsEnd; slot++)
                {
       
     


                    CEquip* Equip = GetEquipManager(cship)->FindByID(*slot);
                    if (CEGun::cast(Equip))
                    {
                        CEGun* cgun = (CEGun*)Equip;

                        AC_Info[iClientID].fCurEstCapacity -= cgun->GunArch()->fPowerUsage;
                      
                    }
                }
            }


            // Possible cheater if est power is less than the minimum
            if ((AC_Info[iClientID].fCurEstCapacity) < AC_Info[iClientID].fMinCapacity && !AC_Info[iClientID].bPowerCheater && AC_Info[iClientID].fMaxCapacity != 0.0f)
            {

                std::string msg = "capacity=" + std::to_string(AC_Info[iClientID].fMaxCapacity)
                    + "<br> charge_rate=" + std::to_string(AC_Info[iClientID].fChargeRate)
                    + "<br> est_capacity=" + std::to_string(AC_Info[iClientID].fCurEstCapacity);
                
                AntiCheat::Reporting::ReportCheater(iClientID, "Power", msg);
                AC_Info[iClientID].bPowerCheater = true;

                AC_Info[iClientID].fCurEstCapacity = 0.0f;
            }
            // Stop est power plant energy getting to high.
            else if (AC_Info[iClientID].fCurEstCapacity > AC_Info[iClientID].fMaxCapacity)
            {
               std::string msg = "capacity=" + std::to_string(AC_Info[iClientID].fMaxCapacity)
                    + "<br> charge_rate=" + std::to_string(AC_Info[iClientID].fChargeRate)
                    + "<br> est_capacity=" + std::to_string(AC_Info[iClientID].fCurEstCapacity);
              // AntiCheat::Reporting::ReportCheater(iClientID, "Power Over Cap ", msg);
            

                 AC_Info[iClientID].fCurEstCapacity = AC_Info[iClientID].fMaxCapacity;
            }
        }
		
    }
	
    // AC Reporting
    namespace Reporting {
	    void ReportCheater(ClientId iClientID, std::string scType, std::string sData) 
	    {
		    std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
            std::wstring wscType = stows(scType);

            std::wstring wscReport;

		    //Get Time
            time_t rawtime;
            struct tm *timeinfo;
            char cTime[80];
            time(&rawtime);
            timeinfo = localtime(&rawtime);
		    strftime(cTime, sizeof(cTime), "%d-%m-%Y-%H-%M-%S", timeinfo);
            std::string sTime(cTime);
            std::wstring wscTime = stows(sTime);

		    //Build Report
            wscReport = wscCharname + L" - " + wscTime + L" - " + wscType;

            //PrintUserCmdText(iClientID, wscReport);
            //return;

            //LogCheat
            std::string reportname = "PATHPATH" + CreateReport(iClientID, wscType, wscTime, stows(sData));
            wscReport = wscReport + stows(reportname);

            //PrintUserCmdText(iClientID, wscReport);
            //Report to Discord
            ShellExecute(NULL, "open", DISCORD_WEBHOOK_CHEATREPORT_FILE, wstos(wscReport).c_str(), NULL, NULL);
	
	
	    }

        std::string CreateReport(ClientId iClientID, std::wstring wscType,std::wstring wscTime, std::wstring wscDETAILS)
        {
            //FLPath
            char szCurDir[MAX_PATH];
            GetCurrentDirectory(sizeof(szCurDir), szCurDir);

            CAccount *acc = Players.FindAccountFromClientID(iClientID);
            std::wstring wscAccDir;
            HkGetAccountDirName(acc, wscAccDir);
            std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;
            std::string scACFile = CHEATREPORT_STORE;
            std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
            std::string Charname = wstos(wscCharname);
            std::string sTime = wstos(wscTime);
            std::wstring wscFilename;
            std::wstring charname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
            HkGetCharFileName(charname, wscFilename);
            std::string scFilename = wstos(wscFilename);
            scACFile = scACFile + scFilename + "_" + sTime + ".html";
            std::ifstream RTPL(std::string(szCurDir) + AC_REPORT_TPL);

            std::string AllCharnames;
            //Get Charnames on ID
            for (int i = 0;; i++) {
                char szBuf[64];
                sprintf(szBuf, "Char%u", i);
                std::string aCharname = IniGetS(scUserFile, "Charnames", szBuf, "");
                Tools::replace_first(aCharname, "-> ", ""); // Charname
                aCharname = Tools::base64_decode(aCharname);
                AllCharnames = AllCharnames + "<br>" + aCharname;
                if (aCharname == "")
                    break;
            }
        

            //TPL
            std::ostringstream fileContentsStream;
            fileContentsStream << std::ifstream(std::string(szCurDir) + AC_REPORT_TPL).rdbuf();
            std::string RTPL_STR = fileContentsStream.str();


            //Prepare Report Template
            std::string sCHARNAME = wstos(wscCharname); // Charname
            std::string sCTIMINGSPEED = IniGetS(scUserFile, "CheatDetecion", "CTD", "0");// Confirmed TimingSpeed Detections

            //Replace PlaceHolder->Content
            Tools::replace_first(RTPL_STR, "{[pos1]}", sCHARNAME); // Charname
            Tools::replace_first(RTPL_STR, "{[pos2]}", sCTIMINGSPEED); // Confirmed TimingSpeed Detections

            //Prepare Report Body Template
            std::string sDATE = wstos(wscTime); // Date
            std::string sTYPE = wstos(wscType); // Confirmed TimingSpeed Detections



            std::string scDETAILS = wstos(wscDETAILS);
            //Replace PlaceHolder->Content
            Tools::replace_first(RTPL_STR, "{[posB1]}", sDATE);       // Date
            Tools::replace_first(RTPL_STR, "{[posB2]}", sTYPE);          // Type
            Tools::replace_first(RTPL_STR, "{[posB3]}", scDETAILS);   // Details
            Tools::replace_first(RTPL_STR, "{[posB4]}", AllCharnames);   // Charnames
            Tools::replace_first(RTPL_STR, "{[posB5]}", sCTIMINGSPEED); // StatsTDC

            //PrintUserCmdText(iClientID, stows(RTPL_STR));

            FILE *f;
            fopen_s(&f, scACFile.c_str(), "at");
            if (!f)
                return "";

            try {
                fprintf(f, RTPL_STR.c_str());

                fprintf(f, "\n");
            } catch (...) {
            }
            fclose(f);


            return scFilename + "_" + sTime + ".html";
        }

    }

    // AC Tools
    namespace DataGrab {
        void CharnameToFLHOOKUSER_FILE(ClientId iClientID) {
            std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
            std::string scCharname = wstos(wscCharname);
            std::string scBase64Charname = "-> " + Tools::base64_encode((const unsigned char *)scCharname.c_str(), scCharname.length());
            char szCurDir[MAX_PATH];
            GetCurrentDirectory(sizeof(szCurDir), szCurDir);
            CAccount *acc = Players.FindAccountFromClientID(iClientID);
            std::wstring wscAccDir;
            HkGetAccountDirName(acc, wscAccDir);
            std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;


            //SaveCharnames        
            for (int i = 0;; i++) {
                char szBuf[64];
                sprintf(szBuf, "Char%u", i);
                std::string Charname = IniGetS(scUserFile, "Charnames", szBuf, "");
                if (Charname == scBase64Charname) {
                    break;        
                }
                if (Charname == "" ) {

                    IniWrite(scUserFile, "Charnames", szBuf, scBase64Charname);
                    break;
                }
            }
        }

    }
	
}