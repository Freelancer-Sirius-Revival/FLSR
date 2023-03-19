#include "main.h"

//Usefull: Archetype::Equipment *item = Archetype::GetEquipment(MountedEquip->iArchID);


namespace Insurance {

	bool Insurance_Module = true;
    std::list<PlayerDied> lPlayerDied;
    std::list<BookInsuranceEvent> lBookInsuranceEvent;
    std::map<std::wstring, std::list<PriceList>> mPriceList;
    float set_fCostPercent;

	void CreateNewInsurance(uint iClientID, bool bFreeInsurance) {
        HK_ERROR err;
        int iCountEquip = 0;

        //Create New Insurance
        //ConPrint(L"NewInsurance\n");
        
        //Player CharfileName
        std::wstring wscCharFileName;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);

        //New PriceList
        std::list<PriceList> lPriceList;

        //Player cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

        //Add mounted Equip to list
        std::list<CARGO_INFO> lstMounted;
        float fValue;
        int iPrice;
        for (auto &cargo : lstCargo) {
            if (!cargo.bMounted)
                continue;

            //Check Archtype
            Archetype::Equipment *eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();
            if (aType == Archetype::SHIELD_GENERATOR ||
                aType == Archetype::THRUSTER || aType == Archetype::LAUNCHER ||
                aType == Archetype::GUN || aType == Archetype::MINE_DROPPER ||
                aType == Archetype::COUNTER_MEASURE_DROPPER || 
                aType == Archetype::CLOAKING_DEVICE) {

                
                const GoodInfo *gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi) {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi) {
                        //float *fResaleFactor = (float *)((char *)hModServer + 0x8AE78);
                        float fItemValue = gi->fPrice; //* (*fResaleFactor);
                        iPrice = (int)fItemValue;

                        fValue += fItemValue;

                        

                        //Save
                        if (iPrice <= 0) {
                            iCountEquip++;
                            lstMounted.push_back(cargo);
                                
                            //AddToPriceList
                            PriceList NewEntry;
                            NewEntry.bFreeInsurance = bFreeInsurance;
                            NewEntry.bItemisFree = true;
                            NewEntry.CARGO_INFO = cargo;
                            NewEntry.GoodInfo = *gi;
                            lPriceList.push_back(NewEntry);
                               

                        }
                        else 
                        {
                            iCountEquip++;
                            lstMounted.push_back(cargo);

                            //AddToPriceList
                            PriceList NewEntry;
                            NewEntry.bFreeInsurance = bFreeInsurance;
                            NewEntry.bItemisFree = false;
                            NewEntry.CARGO_INFO = cargo;
                            NewEntry.GoodInfo = *gi;
                            lPriceList.push_back(NewEntry);
                        }
              
                    }
                }
            }
        }

        //Save Pricelist to Map
		mPriceList[wscCharFileName] = lPriceList;

        //Get PriceList
        std::list<PriceList> lPlayerPriceList = mPriceList[wscCharFileName];
		
        //Set Price
        float fprice = 0.0f;
        float fCalculatedPrice = 0.0f;

        //While List
        std::list<PriceList>::iterator MountedEquipPriceList = lPlayerPriceList.begin();
        //ConPrint(L"Items: %i \n", lPlayerPriceList.size());
        //ConPrint(L"Percent: " + std::to_wstring(Insurance::set_fCostPercent) + L"\n");
        while (MountedEquipPriceList != lPlayerPriceList.end()) {

           //GetData
            GoodInfo gi = MountedEquipPriceList->GoodInfo;
			CARGO_INFO ci = MountedEquipPriceList->CARGO_INFO;

            //Print Data to Console

            //ConPrint(L"Insurance is Free: " + std::to_wstring(MountedEquipPriceList->bFreeInsurance) + L", MountedEquipID: " + std::to_wstring(gi.iArchID) + L", Mounted on Hardpoint: " + stows(ci.hardpoint.value) + L", Price: " + std::to_wstring(gi.fPrice) + L", Equip is Free: " + std::to_wstring(MountedEquipPriceList->bItemisFree) + L"\n");


            if (!MountedEquipPriceList->bItemisFree && !MountedEquipPriceList->bFreeInsurance)
            {
                fprice += gi.fPrice;
                fCalculatedPrice = fCalculatedPrice + ((gi.fPrice / 100.0f) * Insurance::set_fCostPercent);

               // ConPrint(L"New Calculated Price: " + std::to_wstring(fCalculatedPrice) + L"\n");
            }
           

            MountedEquipPriceList++;
        }
  
		//Cast fCalculatedPrice to int
        int iCalculatedPrice = static_cast<int>(fCalculatedPrice);

        //Get Player Cash
        int iCash;
        HkGetCash(ARG_CLIENTID(iClientID), iCash);
        
		//Check if Player has enough Cash
        if (iCalculatedPrice <= iCash) {
            char szCurDir[MAX_PATH];
            GetCurrentDirectory(sizeof(szCurDir), szCurDir);
            std::string scInsuranceStore = std::string(szCurDir) + INSURANCE_STORE;
            std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
            std::string Charname = wstos(wscCharname);
            std::wstring wscFilename;
            HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
            std::string scFilename = wstos(wscFilename);
            
            // Remove Cash
            if ((err = HkAddCash(wscCharname, 0 - iCalculatedPrice)) != HKE_OK) {
                PrintUserCmdText(iClientID, L"ERR Remove cash failed err=" + HkErrGetText(err));
                return;
            }

            if (HkAntiCheat(iClientID) != HKE_OK) {
                PrintUserCmdText(iClientID, L"ERR Insurance-Booking failed");
                AddLog("NOTICE: Possible cheating when book Insurance %s credits from %s ", wstos(ToMoneyStr(iCalculatedPrice)).c_str(), wstos(wscCharname).c_str());
                return;
            }

            //HkSaveChar(iClientID);


            // New Insurance
            IniWrite(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "Charname", scFilename);
            IniWrite(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "Worth", std::to_string(iCalculatedPrice));
            IniWrite(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "EquipCount", std::to_string(iCountEquip));
            if (bFreeInsurance)
                IniWrite(scInsuranceStore + scFilename + ".cfg","INSURANCE", "FreeInsurance", "true");

            //Write Equip to Store
            std::list<PriceList>::iterator MountedEquipPriceList = lPlayerPriceList.begin();
            int Equip = 0;
            while (MountedEquipPriceList != lPlayerPriceList.end()) {

                //GetData
                GoodInfo gi = MountedEquipPriceList->GoodInfo;
                CARGO_INFO ci = MountedEquipPriceList->CARGO_INFO;

                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "bMission", std::to_string(ci.bMission));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "bMounted", std::to_string(ci.bMounted));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "fStatus", std::to_string(ci.fStatus));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "hardpoint", ci.hardpoint.value);
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "iArchID", std::to_string(ci.iArchID));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "iCount", std::to_string(ci.iCount));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "iID", std::to_string(ci.iID));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "fPrice", std::to_string(((gi.fPrice / 100.0f)* Insurance::set_fCostPercent)));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "bItemisFree", std::to_string(MountedEquipPriceList->bItemisFree));


                Equip++;
                MountedEquipPriceList++;
            }

            if (!bFreeInsurance)
                PrintUserCmdText(iClientID, L"Your ship is insured. Upon landing the unspent insurance deposit will be transferred back.");
        } else {
            if (!bFreeInsurance)
                PrintUserCmdText(iClientID, L"Insurance failed. You need to have at least " + ToMoneyStr(iCalculatedPrice) + L" Credits for this.");
        }


        return;
	}

    void UseInsurance(uint iClientID) {
        HK_ERROR err;



        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scInsuranceStore = std::string(szCurDir) + INSURANCE_STORE;
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::string Charname = wstos(wscCharname);
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);


        // Check if Insurance Exists
        if (!insurace_exists(scInsuranceStore + scFilename + ".cfg")) {
            return;
        }

        //Get Data from Store
        std::list<RestoreEquip> loldMounted;

        int Store_iWorth = IniGetI(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "Worth", 0);
        int Store_iCountEquip = IniGetI(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "EquipCount", 0);
        bool bFreeInsurance = IniGetB(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "FreeInsurance", false);

        //Read Insurance from Store
        int Equiploop = 0;
        while (Equiploop <= Store_iCountEquip) {

            //Read Equip
            bool bMission = IniGetB(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "bMission", false);
            bool bMounted = IniGetB(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "bMounted", true);
            float fStatus = IniGetF(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "fStatus", 1);
            std::string hardpoint = IniGetS(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "hardpoint", "");
            uint iArchID = IniGetI(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "iArchID", 0);
            uint iCount = IniGetI(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "iCount", 0);
            uint iID = IniGetI(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "iID", 0);

            //Prepare Data
            CacheString newhardpoint;
            const char* cshardpoint = hardpoint.c_str();
            newhardpoint.value = StringAlloc(cshardpoint, false);

            //Create Equip
            CARGO_INFO NewCargo_Info;
            NewCargo_Info.bMission = bMission;
            NewCargo_Info.bMounted = bMounted;
            NewCargo_Info.fStatus = fStatus;
            NewCargo_Info.hardpoint = newhardpoint;
            NewCargo_Info.iArchID = iArchID;
            NewCargo_Info.iCount = iCount;
            NewCargo_Info.iID = iID;

            RestoreEquip NewEquip;
			NewEquip.CARGO_INFO = NewCargo_Info;
			NewEquip.bItemisFree = IniGetB(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "bItemisFree", true);
			NewEquip.fPrice = IniGetF(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equiploop), "fPrice", 0);
            loldMounted.push_back(NewEquip);

            Equiploop++;
        }

        //Get Player Cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

        // Add mounted Equip to list
        std::list<CARGO_INFO> lstMounted;
        for (auto& cargo : lstCargo) {
            if (!cargo.bMounted)
                continue;

            // Check Archtype
            Archetype::Equipment* eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();
            if (aType == Archetype::SHIELD_GENERATOR ||
                aType == Archetype::THRUSTER || aType == Archetype::LAUNCHER ||
                aType == Archetype::GUN || aType == Archetype::MINE_DROPPER ||
                aType == Archetype::COUNTER_MEASURE_DROPPER ||
                aType == Archetype::CLOAKING_DEVICE) {

                lstMounted.push_back(cargo);
            }
        }

        //Merge Insurance with actual Equip
        std::list<RestoreEquip>::iterator InsuranceEquip = loldMounted.begin();

        float fPrice = 0.0f;
        uint iEquipAdded = 0;
        bool bAdded = false;
        while (InsuranceEquip != loldMounted.end())
        {
            if (!FindHardpointCargolist(lstMounted, InsuranceEquip->CARGO_INFO.hardpoint))
            {
                if (InsuranceEquip->CARGO_INFO.iArchID != 0) {
                    //DEBUG
                    //PrintUserCmdText(iClientID, L"ADDED STUFF");
                    //PrintUserCmdText(iClientID, stows(InsuranceEquip->hardpoint.value));
                    //PrintUserCmdText(iClientID, std::to_wstring(InsuranceEquip->iArchID));
                    std::string phardpoint = InsuranceEquip->CARGO_INFO.hardpoint.value;
                    HkAddEquip(ARG_CLIENTID(iClientID), InsuranceEquip->CARGO_INFO.iArchID, phardpoint);
                    
                    bAdded = true;

                    if (!InsuranceEquip->bItemisFree && !bFreeInsurance)
                    {
                        fPrice += InsuranceEquip->fPrice;
                    }

                }

            }

            InsuranceEquip++;
        }

        // Add Cash
        if(!bFreeInsurance)
        { 
            int CashBack = Store_iWorth - static_cast<int>(fPrice);

            if ((err = HkAddCash(wscCharname, 0 + CashBack)) != HKE_OK) {
                PrintUserCmdText(iClientID, L"ERR Add cash failed err=" + HkErrGetText(err));
            }

			//PrintUserCmdText(iClientID, L"Insurance Deposit: " + std::to_wstring(Store_iWorth) + L". Cashback: " + std::to_wstring(CashBack));
        }
        
        //PlayerFeedback
        if (!bFreeInsurance) {
            if (bAdded) {
                PrintUserCmdText(iClientID, L"Your lost Equipment was replaced by your insurance. The unspent insurance deposit was transferred back.");
			}
            else
            {
                PrintUserCmdText(iClientID, L"The unspent insurance deposit was transferred back.");
            }
        }
        
        //DeleteInsurance
        std::string sInsurancepath = scInsuranceStore + scFilename + ".cfg";
        remove(sInsurancepath.c_str());

        //Bypass AntiCheat
        struct PlayerData *pPD = 0;
        while (pPD = Players.traverse_active(pPD)) {
            uint piClientID = HkGetClientIdFromPD(pPD);
            if (piClientID == iClientID) {

                char *szClassPtr;
                memcpy(&szClassPtr, &Players, 4);
                szClassPtr += 0x418 * (iClientID - 1);
                ulong lCRC;
                __asm
                    {
				pushad
				mov ecx, [szClassPtr]
				call [CRCAntiCheat_FLSR]
				mov [lCRC], eax
				popad
                    }
                memcpy(szClassPtr + 0x320, &lCRC, 4);
                break;
            }
        }
    }

    void PlayerDiedEvent(bool bDied, uint iClientID) {
        // New PlayerDied Event
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        bool Playerfound = false;
        
        //Edit List
        std::list<PlayerDied>::iterator PlayerDiedList = lPlayerDied.begin();
        while (PlayerDiedList != lPlayerDied.end()) {
            if (PlayerDiedList->wscCharname == wscCharname) {
                // Update the DiedList
                Insurance::PlayerDied NewDeath;
                NewDeath.wscCharname = wscCharname;
                NewDeath.bDied = bDied;
                lPlayerDied.erase(PlayerDiedList);
                lPlayerDied.push_back(NewDeath);
                Playerfound = true;
                //DEBUG
                //PrintUserCmdText(iClientID, L"entry");
                //PrintUserCmdText(iClientID, std::to_wstring(bDied));
                return;
            }

            PlayerDiedList++;
        }

        //Add to List
        if (Playerfound == false) 
        {
            Insurance::PlayerDied NewDeath;
            NewDeath.wscCharname = wscCharname;
            NewDeath.bDied = bDied;
            lPlayerDied.push_back(NewDeath);
            //DEBUG
            //PrintUserCmdText(iClientID, L"no entry");
            //PrintUserCmdText(iClientID, std::to_wstring(bDied));


        }
    }

    bool CheckPlayerDied(uint iClientID) {
        // New PlayerDied Event
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        bool bReturn = false;
        std::list<PlayerDied>::iterator PlayerDiedList = lPlayerDied.begin();
        while (PlayerDiedList != lPlayerDied.end()) {
            if (PlayerDiedList->wscCharname == wscCharname) {
                bReturn = PlayerDiedList->bDied;
                //DEBUG
                //PrintUserCmdText(iClientID, L"DEATHLIST found.->"+std::to_wstring(bReturn));
            }

            PlayerDiedList++;
        }
        return bReturn;
    }

    //Find Hardpoint in Cargolist
    bool FindHardpointCargolist( std::list<CARGO_INFO> &cargolist, CacheString &hardpoint) {
        std::string shardpoint = hardpoint.value;


        for (const auto &p : cargolist) {
            std::string phardpoint = p.hardpoint.value;
            if (phardpoint == shardpoint) {
                return true;
            }
        }
        return false;
    }

    //Book Insurance
    void BookInsurance(uint iClientID, bool bFreeItem) {
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        bool Playerfound = false;

        // Edit List
        std::list<BookInsuranceEvent>::iterator PlayerBooked = lBookInsuranceEvent.begin();
        while (PlayerBooked != lBookInsuranceEvent.end()) {
            if (PlayerBooked->wscCharname == wscCharname) {
                // Update the PlayerBooked List
                Insurance::BookInsuranceEvent NewInsuranceBook;
                NewInsuranceBook.wscCharname = wscCharname;
                NewInsuranceBook.bFreeItem = bFreeItem;
                //PrintUserCmdText(iClientID, std::to_wstring(NewInsuranceBook.bFreeItem));
                lBookInsuranceEvent.erase(PlayerBooked);
                lBookInsuranceEvent.push_back(NewInsuranceBook);
                Playerfound = true;
                return;
            }

            PlayerBooked++;
        }

        // Add to List
        if (Playerfound == false) {
            Insurance::BookInsuranceEvent NewInsuranceBook;
            NewInsuranceBook.wscCharname = wscCharname;
            NewInsuranceBook.bFreeItem = bFreeItem;
            //PrintUserCmdText(iClientID, std::to_wstring(NewInsuranceBook.bFreeItem));

            lBookInsuranceEvent.push_back(NewInsuranceBook);
        }
    }

    //CheckInsuranceBooked
    std::pair<bool, bool> CheckInsuranceBooked(uint iClientID) {
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        bool bReturn = false;
        bool bFreeItem = true;
        std::list<BookInsuranceEvent>::iterator PlayerBooked = lBookInsuranceEvent.begin();
        while (PlayerBooked != lBookInsuranceEvent.end()) {
            if (PlayerBooked->wscCharname == wscCharname) {
                bReturn = true;
                bFreeItem = PlayerBooked->bFreeItem;
                lBookInsuranceEvent.erase(PlayerBooked);
                break;
            }

            PlayerBooked++;
        }
        return std::make_pair(bReturn, bFreeItem);
    }

    std::wstring CalcInsurance(uint iClientID, bool bPlayerCMD, bool bFreeInsurance)
    {
        HK_ERROR err;
        int iCountEquip = 0;

        //Create New Insurance
        //ConPrint(L"NewInsurance\n");

        //Player CharfileName
        std::wstring wscCharFileName;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);
        
        //New PriceList
        std::list<PriceList> lPriceList;

        //Player cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

        //Add mounted Equip to list
        std::list<CARGO_INFO> lstMounted;
        float fValue;
        int iPrice;
        for (auto& cargo : lstCargo) {
            if (!cargo.bMounted)
                continue;

            //Check Archtype
            Archetype::Equipment* eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();
            if (aType == Archetype::SHIELD_GENERATOR ||
                aType == Archetype::THRUSTER || aType == Archetype::LAUNCHER ||
                aType == Archetype::GUN || aType == Archetype::MINE_DROPPER ||
                aType == Archetype::COUNTER_MEASURE_DROPPER ||
                aType == Archetype::CLOAKING_DEVICE) {


                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi) {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi) {
                        //float *fResaleFactor = (float *)((char *)hModServer + 0x8AE78);
                        float fItemValue = gi->fPrice; //* (*fResaleFactor);
                        iPrice = (int)fItemValue;

                        fValue += fItemValue;


                        //Save
                        if (iPrice <= 0) {
                            iCountEquip++;
                            lstMounted.push_back(cargo);

                            //AddToPriceList
                            PriceList NewEntry;
                            NewEntry.bFreeInsurance = bFreeInsurance;
                            NewEntry.bItemisFree = true;
                            NewEntry.CARGO_INFO = cargo;
                            NewEntry.GoodInfo = *gi;
                            lPriceList.push_back(NewEntry);


                        }
                        else
                        {
                            iCountEquip++;
                            lstMounted.push_back(cargo);

                            //AddToPriceList
                            PriceList NewEntry;
                            NewEntry.bFreeInsurance = bFreeInsurance;
                            NewEntry.bItemisFree = false;
                            NewEntry.CARGO_INFO = cargo;
                            NewEntry.GoodInfo = *gi;
                            lPriceList.push_back(NewEntry);
                        }

                    }
                }
            }
        }

        //Save Pricelist to Map
        mPriceList[wscCharFileName] = lPriceList;

        //Get PriceList
        std::list<PriceList> lPlayerPriceList = mPriceList[wscCharFileName];

        //Set Price
        float fprice = 0.0f;
        float fCalculatedPrice = 0.0f;

        //While List
        std::list<PriceList>::iterator MountedEquipPriceList = lPlayerPriceList.begin();
        //ConPrint(L"Items: %i \n", lPlayerPriceList.size());
        //ConPrint(L"Percent: " + std::to_wstring(Insurance::set_fCostPercent) + L"\n");
        while (MountedEquipPriceList != lPlayerPriceList.end()) {

            //GetData
            GoodInfo gi = MountedEquipPriceList->GoodInfo;
            CARGO_INFO ci = MountedEquipPriceList->CARGO_INFO;

            //Print Data to Console

            //ConPrint(L"Insurance is Free: " + std::to_wstring(MountedEquipPriceList->bFreeInsurance) + L", MountedEquipID: " + std::to_wstring(gi.iArchID) + L", Mounted on Hardpoint: " + stows(ci.hardpoint.value) + L", Price: " + std::to_wstring(gi.fPrice) + L", Equip is Free: " + std::to_wstring(MountedEquipPriceList->bItemisFree) + L"\n");


            if (!MountedEquipPriceList->bItemisFree && !MountedEquipPriceList->bFreeInsurance)
            {
                fprice += gi.fPrice;
                fCalculatedPrice = fCalculatedPrice + ((gi.fPrice / 100.0f) * Insurance::set_fCostPercent);

                // ConPrint(L"New Calculated Price: " + std::to_wstring(fCalculatedPrice) + L"\n");
            }


            MountedEquipPriceList++;
        }

        //Cast fCalculatedPrice to int
        int iCalculatedPrice = static_cast<int>(fCalculatedPrice);


        if (!bPlayerCMD)
        {
            PrintUserCmdText(iClientID, L"You applied for an insurance. This will deposit approximately " + ToMoneyStr(iCalculatedPrice) + L" Credits as soon as you launch to space.");
        }
        else {
            PrintUserCmdText(iClientID, L"An insurance will deposit approximately " + ToMoneyStr(iCalculatedPrice) + L" Credits as soon as you launch to space.");

        }
        return ToMoneyStr(iCalculatedPrice);
    }

    //Check File Exists
    bool insurace_exists(const std::string &name) 
    {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }

    void ReNewInsurance(uint iClientID) {
        uint ship;
        pub::Player::GetShip(iClientID, ship);

        // Check On/Off
        CAccount *acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        std::string Charname = wstos(wscCharname);
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);


        std::string sAutoInsurance = IniGetS(scUserFile, scFilename, "INSURANCE-State", "no");


        if (sAutoInsurance == "yes") {
            if (!ship) {
                if (Insurance::Insurance_Module) {
                    
					//Too much Spam
                    //Insurance::CalcInsurance(iClientID);
                    Insurance::BookInsurance(iClientID, false);
                }
            } else {
                PrintUserCmdText(iClientID, L"ERR: You can only renew an insurance when docked.");
            }
        } else {
            if (!ship) {
                if (Insurance::Insurance_Module) {
                    Insurance::BookInsurance(iClientID, true);
                }
            }    
        }
    }
}