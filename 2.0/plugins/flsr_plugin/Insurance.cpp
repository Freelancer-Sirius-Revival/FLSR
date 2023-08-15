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

        // Add mounted Equip to list
        std::list<CARGO_INFO> lstMounted;
        std::list<CARGO_INFO> lstAdditional;
        float fValue;
        int iPrice;
        for (auto const& cargo : lstCargo) {
            Archetype::Equipment const* eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();

            if (cargo.bMounted && isInsurableClass(aType, iClientID)) {

                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi) {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi) {
                        float fItemValue = gi->fPrice;
                        iPrice = (int)fItemValue;

                        fValue += fItemValue;

                        // Save
                        iCountEquip++;
                        lstMounted.push_back(cargo);

                        // AddToPriceList
                        PriceList NewEntry;
                        NewEntry.bFreeInsurance = bFreeInsurance;
                        NewEntry.bItemisFree = (iPrice <= 0);
                        NewEntry.CARGO_INFO = cargo;
                        NewEntry.GoodInfo = *gi;
                        NewEntry.aClassType = aType;
                        NewEntry.bIsAmmo = false;
                        lPriceList.push_back(NewEntry);

                    }
                }
            }
            else if (isAmmoClass(aType, iClientID)) {

                //  ConPrint(L"found Ammo item\n");
                  //ConPrint(L"bMounted: %d, iCount: %u, ArchType: %u, ArchID: %u\n", cargo.bMounted, cargo.iCount, aType, cargo.iArchID);

                Archetype::Ship* ship_ID = Archetype::GetShip(Players[iClientID].iShipArchetype);
                uint NanobotsID;
                pub::GetGoodID(NanobotsID, "ge_s_repair_01");
                uint MaxNanobots = ship_ID->iMaxNanobots;
                uint ShieldBatsID;
                pub::GetGoodID(ShieldBatsID, "ge_s_battery_01");
                uint MaxShieldBats = ship_ID->iMaxShieldBats;
                uint MaxHoldSize = ship_ID->fHoldSize;

                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi) {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi) {

                        if (gi->iArchID == NanobotsID)
                        {
                            if (cargo.iCount > MaxNanobots)
                            {

                                float fItemValue = gi->fPrice;
                                iPrice = (int)fItemValue;
                                fValue += fItemValue;
                                // Save
                                iCountEquip++;
                                lstMounted.push_back(cargo);

                                CARGO_INFO insured_NB = cargo;
                                insured_NB.iCount = MaxNanobots;

                                // AddToPriceList
                                PriceList NewEntry;
                                NewEntry.bFreeInsurance = bFreeInsurance;
                                NewEntry.bItemisFree = (iPrice <= 0);
                                NewEntry.CARGO_INFO = insured_NB;
                                NewEntry.GoodInfo = *gi;
                                NewEntry.aClassType = aType;
                                NewEntry.bIsAmmo = true;
                                lPriceList.push_back(NewEntry);

                            }


                            else
                            {
                                float fItemValue = gi->fPrice;
                                iPrice = (int)fItemValue;
                                fValue += fItemValue;
                                // Save
                                iCountEquip++;
                                lstMounted.push_back(cargo);

                                // AddToPriceList
                                PriceList NewEntry;
                                NewEntry.bFreeInsurance = bFreeInsurance;
                                NewEntry.bItemisFree = (iPrice <= 0);
                                NewEntry.CARGO_INFO = cargo;
                                NewEntry.GoodInfo = *gi;
                                NewEntry.aClassType = aType;
                                NewEntry.bIsAmmo = true;
                                lPriceList.push_back(NewEntry);

                            }

                        }




                        else if (gi->iArchID == ShieldBatsID)
                        {
                            if (cargo.iCount > MaxShieldBats)
                            {

                                float fItemValue = gi->fPrice;
                                iPrice = (int)fItemValue;
                                fValue += fItemValue;
                                // Save
                                iCountEquip++;
                                lstMounted.push_back(cargo);

                                CARGO_INFO insured_NB = cargo;
                                insured_NB.iCount = MaxShieldBats;

                                // AddToPriceList
                                PriceList NewEntry;
                                NewEntry.bFreeInsurance = bFreeInsurance;
                                NewEntry.bItemisFree = (iPrice <= 0);
                                NewEntry.CARGO_INFO = insured_NB;
                                NewEntry.GoodInfo = *gi;
                                NewEntry.aClassType = aType;
                                NewEntry.bIsAmmo = true;
                                lPriceList.push_back(NewEntry);

                            }


                            else
                            {
                                float fItemValue = gi->fPrice;
                                iPrice = (int)fItemValue;
                                fValue += fItemValue;
                                // Save
                                iCountEquip++;
                                lstMounted.push_back(cargo);

                                // AddToPriceList
                                PriceList NewEntry;
                                NewEntry.bFreeInsurance = bFreeInsurance;
                                NewEntry.bItemisFree = (iPrice <= 0);
                                NewEntry.CARGO_INFO = cargo;
                                NewEntry.GoodInfo = *gi;
                                NewEntry.aClassType = aType;
                                NewEntry.bIsAmmo = true;
                                lPriceList.push_back(NewEntry);

                            }

                        }



                        else
                        {

                            float fItemValue = gi->fPrice;
                            iPrice = (int)fItemValue;

                            fValue += fItemValue;

                            // Save
                            iCountEquip++;
                            lstMounted.push_back(cargo);

                            // AddToPriceList
                            PriceList NewEntry;
                            NewEntry.bFreeInsurance = bFreeInsurance;
                            NewEntry.bItemisFree = (iPrice <= 0);
                            NewEntry.CARGO_INFO = cargo;
                            NewEntry.GoodInfo = *gi;
                            NewEntry.aClassType = aType;
                            NewEntry.bIsAmmo = true;
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

          //  ConPrint(L"Insurance is Free: " + std::to_wstring(MountedEquipPriceList->bFreeInsurance) + L", MountedEquipID: " + std::to_wstring(gi.iArchID) + L", Mounted on Hardpoint: " + stows(ci.hardpoint.value) + L", Price: " + std::to_wstring(gi.fPrice) + L", Equip is Free: " + std::to_wstring(MountedEquipPriceList->bItemisFree) + L"\n");


            if (!MountedEquipPriceList->bItemisFree  /* && !MountedEquipPriceList->bFreeInsurance*/ && !MountedEquipPriceList->bIsAmmo)
            {
                fprice += gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount;
                fCalculatedPrice = fCalculatedPrice + (((gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount) / 100.0f) * Insurance::set_fCostPercent);
                ConPrint(L"fCalculatedPrice calc: " + std::to_wstring(fCalculatedPrice) + L"\n");

            }
            else if (!MountedEquipPriceList->bItemisFree  /* && !MountedEquipPriceList->bFreeInsurance */ && MountedEquipPriceList->bIsAmmo)
            {
                fprice += gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount;
                fCalculatedPrice = fCalculatedPrice + (gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount);
            }



            ++MountedEquipPriceList;
        }


        ConPrint(L"fCalculatedPrice final: " + std::to_wstring(fCalculatedPrice) + L"\n");

        //Cast fCalculatedPrice to int
        int iCalculatedPrice = static_cast<int>(fCalculatedPrice);

        //Get Player Cash
        int iCash;
        HkGetCash(ARG_CLIENTID(iClientID), iCash);

        //Check if Player has enough Cash
        if (iCalculatedPrice <= iCash) {
            char szCurDir[MAX_PATH];
            GetCurrentDirectory(sizeof(szCurDir), szCurDir);
            std::string scInsuranceStore = std::string(szCurDir) + Globals::INSURANCE_STORE;
            std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
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

            //Test
            iCountEquip--;

            // New Insurance
            IniWrite(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "Charname", scFilename);
            IniWrite(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "Worth", std::to_string(iCalculatedPrice));
            IniWrite(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "EquipCount", std::to_string(iCountEquip));
            if (bFreeInsurance)
                IniWrite(scInsuranceStore + scFilename + ".cfg", "INSURANCE", "FreeInsurance", "true");

            //Write Equip to Store
            std::list<PriceList>::iterator MountedEquipPriceList = lPlayerPriceList.begin();
            int Equip = 0;
            while (MountedEquipPriceList != lPlayerPriceList.end()) {

                //GetData
                GoodInfo gi = MountedEquipPriceList->GoodInfo;
                CARGO_INFO ci = MountedEquipPriceList->CARGO_INFO;
                std::string classType = std::to_string(static_cast<int>(MountedEquipPriceList->aClassType));


                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "bMission", std::to_string(ci.bMission));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "bMounted", std::to_string(ci.bMounted));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "fStatus", std::to_string(ci.fStatus));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "hardpoint", ci.hardpoint.value);
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "iArchID", std::to_string(ci.iArchID));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "iCount", std::to_string(ci.iCount));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "iID", std::to_string(ci.iID));

                //Price
                if (!MountedEquipPriceList->bIsAmmo)
                {
                    IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "fPrice", std::to_string(((gi.fPrice / 100.0f) * Insurance::set_fCostPercent)));
                }
                else
                {
                    IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "fPrice", std::to_string(gi.fPrice));
                }


                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "bItemisFree", std::to_string(MountedEquipPriceList->bItemisFree));
                IniWrite(scInsuranceStore + scFilename + ".cfg", "Equip-" + std::to_string(Equip), "ClassType", classType);



                Equip++;
                MountedEquipPriceList++;
            }

            if (iCalculatedPrice > 0)
                PrintUserCmdText(iClientID, L"Your ship is insured with total worth of $" + ToMoneyStr(iCalculatedPrice) + L" .Upon landing the unspent insurance deposit will be transferred back.");
        }
        else {
            // if (!bFreeInsurance)
            PrintUserCmdText(iClientID, L"Insurance failed. You need to have at least " + ToMoneyStr(iCalculatedPrice) + L" Credits for this.");
        }


        return;
    }

    void UseInsurance(uint iClientID) {
        HK_ERROR err;



        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scInsuranceStore = std::string(szCurDir) + Globals::INSURANCE_STORE;
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
        std::list<RestoreEquip> lInsuredEquip;

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
            lInsuredEquip.push_back(NewEquip);

            Equiploop++;
        }

        // Get Player Cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);


        ConPrint(L"Hold Size: " + std::to_wstring(iRemHoldSize) + L"\n");
        // Initialize the lists to store mounted and additional equipment
        std::list<CARGO_INFO> lstMounted;

        for (auto const& cargo : lstCargo) {
            Archetype::Equipment const* eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();

            if (cargo.bMounted &&
                (aType == Archetype::SHIELD_GENERATOR ||
                    aType == Archetype::THRUSTER ||
                    aType == Archetype::LAUNCHER ||
                    aType == Archetype::GUN ||
                    aType == Archetype::MINE_DROPPER ||
                    aType == Archetype::COUNTER_MEASURE_DROPPER ||
                    aType == Archetype::CLOAKING_DEVICE)) {

                lstMounted.push_back(cargo);
            }
            else if (aType == Archetype::REPAIR_KIT ||
                aType == Archetype::SHIELD_BATTERY ||
                aType == Archetype::MUNITION ||
                aType == Archetype::MINE ||
                aType == Archetype::COUNTER_MEASURE)
            {


                lstMounted.push_back(cargo);
            }

        }

        /*
        ConPrint(L"Insured Equip: \n");
        //Print lInsuredEquip to Console in a Loop
        for (auto const& equip : lInsuredEquip) {
            ConPrint(L"Equip: " + stows(equip.CARGO_INFO.hardpoint.value) + L" " + std::to_wstring(equip.CARGO_INFO.iArchID) + L" " + std::to_wstring(equip.CARGO_INFO.iCount) + L" " + std::to_wstring(equip.CARGO_INFO.iID) + L" " + std::to_wstring(equip.CARGO_INFO.fStatus) + L" " + std::to_wstring(equip.bItemisFree) + L" " + std::to_wstring(equip.fPrice) + L"\n");
        }

        ConPrint(L"Mounted Equip: \n");
        //Print lstMounted to Console in a Loop
        for (auto const& equip : lstMounted) {
            ConPrint(L"Equip: " + stows(equip.hardpoint.value) + L" " + std::to_wstring(equip.iArchID) + L" " + std::to_wstring(equip.iCount) + L" " + std::to_wstring(equip.iID) + L" " + std::to_wstring(equip.fStatus) + L" " + std::to_wstring(equip.bMounted) + L" " + std::to_wstring(equip.bMission) + L"\n");
        }
        */

        //Merge Insurance with actual Equip
        float fPrice = 0.0f;
        //uint iEquipAdded = 0;
        bool bAdded = false;
        auto iterInsuranceEquip = lInsuredEquip.begin();
        while (iterInsuranceEquip != lInsuredEquip.end())
        {
            bool bFound = false;

            Archetype::Equipment const* eq = Archetype::GetEquipment(iterInsuranceEquip->CARGO_INFO.iArchID);


            auto itermountedEquip = lstMounted.begin();
            while (itermountedEquip != lstMounted.end())
            {
                if (itermountedEquip->iArchID == iterInsuranceEquip->CARGO_INFO.iArchID)
                {
                    bFound = true;

                    if (itermountedEquip->iCount < iterInsuranceEquip->CARGO_INFO.iCount)
                    {
                        uint itemsToAdd = iterInsuranceEquip->CARGO_INFO.iCount - itermountedEquip->iCount;


                        if (auto aType = eq->get_class_type();
                            aType == Archetype::REPAIR_KIT ||
                            aType == Archetype::SHIELD_BATTERY ||
                            aType == Archetype::MUNITION ||
                            aType == Archetype::MINE ||
                            aType == Archetype::COUNTER_MEASURE)
                        {

                            Tools::FLSRHkAddCargo(wscCharname, iterInsuranceEquip->CARGO_INFO.iArchID, itemsToAdd, false);
                            ConPrint(L"Added AMMO %u, %u to %s\n", itemsToAdd, iterInsuranceEquip->CARGO_INFO.iArchID, wscCharname.c_str());
                            bAdded = true;

                        }
                        else {
                            std::string phardpoint = iterInsuranceEquip->CARGO_INFO.hardpoint.value;
                            HkAddEquip(ARG_CLIENTID(iClientID), iterInsuranceEquip->CARGO_INFO.iArchID, phardpoint);
                            //ConPrint(L"Added WEAPON %u, %u to %s\n", itemsToAdd, iterInsuranceEquip->CARGO_INFO.iArchID, wscCharname.c_str());
                            bAdded = true;

                        }




                        if (!iterInsuranceEquip->bItemisFree && bAdded)
                        {
                            float newfItemPrice = iterInsuranceEquip->fPrice * itemsToAdd;
                            fPrice += newfItemPrice;
                        }
                    }
                }

                if (bFound)
                    break;

                itermountedEquip++;
            }

            if (!bFound)
            {
                // Das Equipment wurde nicht in lstMounted gefunden, f�gen Sie es hinzu
                //ConPrint(L"not found\n");

                // ConPrint InsuranceEquip Size
                //ConPrint(L"InsuranceEquip Size: %u\n", iterInsuranceEquip->CARGO_INFO.iCount);

                if (auto aType = eq->get_class_type();
                    aType == Archetype::REPAIR_KIT ||
                    aType == Archetype::SHIELD_BATTERY ||
                    aType == Archetype::MUNITION ||
                    aType == Archetype::MINE ||
                    aType == Archetype::COUNTER_MEASURE)
                {
                    //ConPrint(L"Created AMMO %u, %u to %s\n", iterInsuranceEquip->CARGO_INFO.iCount, iterInsuranceEquip->CARGO_INFO.iArchID, wscCharname.c_str());

                    Tools::FLSRHkAddCargo(wscCharname, iterInsuranceEquip->CARGO_INFO.iArchID, iterInsuranceEquip->CARGO_INFO.iCount, false);
                    bAdded = true;

                }
                else {
                    std::string phardpoint = iterInsuranceEquip->CARGO_INFO.hardpoint.value;
                    HkAddEquip(ARG_CLIENTID(iClientID), iterInsuranceEquip->CARGO_INFO.iArchID, phardpoint);
                    //ConPrint(L"Created WEAPON %u, %u to %s\n", iterInsuranceEquip->CARGO_INFO.iCount, iterInsuranceEquip->CARGO_INFO.iArchID, wscCharname.c_str());
                    bAdded = true;

                }



                if (!iterInsuranceEquip->bItemisFree && bAdded)
                {
                    float newfItemPrice = iterInsuranceEquip->fPrice * iterInsuranceEquip->CARGO_INFO.iCount;
                    fPrice += newfItemPrice;
                }

            }



            iterInsuranceEquip++;
        }

        // Add Cash

        int CashBack = Store_iWorth - static_cast<int>(fPrice);

        if ((err = HkAddCash(wscCharname, 0 + CashBack)) != HKE_OK) {
            PrintUserCmdText(iClientID, L"ERR Add cash failed err=" + HkErrGetText(err));
        }

        //PrintUserCmdText(iClientID, L"Insurance Deposit: " + std::to_wstring(Store_iWorth) + L". Cashback: " + std::to_wstring(CashBack));


    //Update CRC
        if (bAdded)
        {

            char* szClassPtr;
            memcpy(&szClassPtr, &Players, 4);
            szClassPtr += 0x418 * (iClientID - 1);
            ulong lCRC;
            __asm
            {
                pushad
                mov ecx, [szClassPtr]
                call[CRCAntiCheat_FLSR]
                mov[lCRC], eax
                popad
            }
            memcpy(szClassPtr + 0x320, &lCRC, 4);
        }

        //PlayerFeedback

        if (bAdded) {
            PrintUserCmdText(iClientID, L"Your lost Equipment was replaced by your insurance. The unspent insurance deposit was transferred back.");
        }
        else
        {
            PrintUserCmdText(iClientID, L"The unspent insurance deposit was transferred back.");
        }


        //DeleteInsurance
        std::string sInsurancepath = scInsuranceStore + scFilename + ".cfg";
        remove(sInsurancepath.c_str());
    }

    void PlayerDiedEvent(bool bDied, uint iClientID) {
        // New PlayerDied Event
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
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
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
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
    bool FindHardpointCargolist(std::list<CARGO_INFO>& cargolist, CacheString& hardpoint) {
        std::string shardpoint = hardpoint.value;


        for (const auto& p : cargolist) {
            std::string phardpoint = p.hardpoint.value;
            if (phardpoint == shardpoint) {
                return true;
            }
        }
        return false;
    }

    //Book Insurance
    void BookInsurance(uint iClientID, bool bFreeItem) {
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
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
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
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

    int CalcInsurance(int iClientID)
    {

        //Player CharfileName
        std::wstring wscCharFileName;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);

        //New PriceList
        std::list<PriceList> lPriceList;

        //Player cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

        // Add mounted Equip to list
        std::list<CARGO_INFO> lstMounted;
        std::list<CARGO_INFO> lstAdditional;
        float fValue = 0;
        int iPrice = 0;
        for (auto const& cargo : lstCargo) {
            Archetype::Equipment const* eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();

            if (cargo.bMounted && isInsurableClass(aType, iClientID)) {

                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi) {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi) {
                        float fItemValue = gi->fPrice;
                        iPrice = (int)fItemValue;

                        fValue += fItemValue;

                        // Save
                        lstMounted.push_back(cargo);

                        // AddToPriceList
                        PriceList NewEntry;
                        NewEntry.bItemisFree = (iPrice <= 0);
                        NewEntry.CARGO_INFO = cargo;
                        NewEntry.GoodInfo = *gi;
                        NewEntry.aClassType = aType;
                        NewEntry.bIsAmmo = false;
                        lPriceList.push_back(NewEntry);

                    }
                }
            }
            else if (isAmmoClass(aType, iClientID)) {

                //ConPrint(L"found Ammo item\n");
                //ConPrint(L"bMounted: %d, iCount: %u, ArchType: %u, ArchID: %u\n", cargo.bMounted, cargo.iCount, aType, cargo.iArchID);

                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi) {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi) {
                        float fItemValue = gi->fPrice;
                        iPrice = (int)fItemValue;

                        fValue += fItemValue;

                        // Save
                        lstMounted.push_back(cargo);

                        // AddToPriceList
                        PriceList NewEntry;
                        NewEntry.bItemisFree = (iPrice <= 0);
                        NewEntry.CARGO_INFO = cargo;
                        NewEntry.GoodInfo = *gi;
                        NewEntry.aClassType = aType;
                        NewEntry.bIsAmmo = true;
                        lPriceList.push_back(NewEntry);
                    }
                }
            }
        }

        //Set Price
        float fprice = 0.0f;
        float fCalculatedPrice = 0.0f;

        //While List
        std::list<PriceList>::iterator MountedEquipPriceList = lPriceList.begin();
        //ConPrint(L"Items: %i \n", lPlayerPriceList.size());
        //ConPrint(L"Percent: " + std::to_wstring(Insurance::set_fCostPercent) + L"\n");
        while (MountedEquipPriceList != lPriceList.end()) {

            //GetData
            GoodInfo gi = MountedEquipPriceList->GoodInfo;
            CARGO_INFO ci = MountedEquipPriceList->CARGO_INFO;

            //Print Data to Console

            //ConPrint(L"Insurance is Free: " + std::to_wstring(MountedEquipPriceList->bFreeInsurance) + L", MountedEquipID: " + std::to_wstring(gi.iArchID) + L", Mounted on Hardpoint: " + stows(ci.hardpoint.value) + L", Price: " + std::to_wstring(gi.fPrice) + L", Equip is Free: " + std::to_wstring(MountedEquipPriceList->bItemisFree) + L"\n");


            if (!MountedEquipPriceList->bItemisFree /* && !MountedEquipPriceList->bFreeInsurance */ && !MountedEquipPriceList->bIsAmmo)
            {
                fprice += gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount;
                fCalculatedPrice = fCalculatedPrice + (((gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount) / 100.0f) * Insurance::set_fCostPercent);
            }
            else if (!MountedEquipPriceList->bItemisFree /* && !MountedEquipPriceList->bFreeInsurance*/ && MountedEquipPriceList->bIsAmmo)
            {
                fprice += gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount;
                fCalculatedPrice = fCalculatedPrice + (gi.fPrice * MountedEquipPriceList->CARGO_INFO.iCount);
            }



            MountedEquipPriceList++;
        }

        //Cast fCalculatedPrice to int
        int iCalculatedPrice = static_cast<int>(fCalculatedPrice);

        return iCalculatedPrice;
    }




    //Check File Exists
    bool insurace_exists(const std::string& name)
    {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }

    void ReNewInsurance(uint iClientID) {
        uint ship;
        pub::Player::GetShip(iClientID, ship);

        // Check On/Off
        CAccount* acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        std::string scUserFile = scAcctPath + wstos(wscAccDir) + Globals::FLHOOKUSER_FILE;
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
        std::string Charname = wstos(wscCharname);
        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);

        std::string sAutoInsurance = IniGetS(scUserFile, scFilename, "AutoInsurance", "OFF");


        if (sAutoInsurance == "ON") {
            if (!ship) {
                if (Insurance::Insurance_Module) {
                    Insurance::BookInsurance(iClientID, false);
                }
            }
            else {
                PrintUserCmdText(iClientID, L"ERR: You can only renew an insurance when docked.");
            }
        }
        else {
            if (!ship) {
                if (Insurance::Insurance_Module) {
                    Insurance::BookInsurance(iClientID, true);
                }
            }
        }
    }

    bool isAmmoClass(Archetype::AClassType aType, uint iClientID)
    {
        // Charfilename
        CAccount* acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        std::string scUserFile = scAcctPath + wstos(wscAccDir) + Globals::FLHOOKUSER_FILE;

        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::string scCharFilename = wstos(wscCharFilename);

        // Lese die INI-Werte
        bool bMines = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Mines", "OFF") == "ON");
        bool bProjectiles = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Projectiles", "OFF") == "ON");
        bool bCountermeasures = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Countermeasures", "OFF") == "ON");
        bool bShieldBatteries = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "ShieldBatteries", "OFF") == "ON");
        bool bNanobots = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Nanobots", "OFF") == "ON");
        bool bEquipment = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Equipment", "OFF") == "ON");

        // Berechne die insuranceMask
        InsuranceType insuranceMask = InsuranceType::None;

        if (bMines)
            insuranceMask |= InsuranceType::Mines;
        if (bProjectiles)
            insuranceMask |= InsuranceType::Projectiles;
        if (bCountermeasures)
            insuranceMask |= InsuranceType::Countermeasures;
        if (bShieldBatteries)
            insuranceMask |= InsuranceType::ShieldBatteries;
        if (bNanobots)
            insuranceMask |= InsuranceType::Nanobots;
        if (bEquipment)
            insuranceMask |= InsuranceType::Equipment;

        // Pr�fen, ob alle sechs Typen festgelegt sind
        if (bool allTypesSet = (bMines && bProjectiles && bCountermeasures && bShieldBatteries && bNanobots && bEquipment))
        {
            if (aType == Archetype::MINE || aType == Archetype::MUNITION ||
                aType == Archetype::COUNTER_MEASURE || aType == Archetype::SHIELD_BATTERY ||
                aType == Archetype::REPAIR_KIT)
            {
                insuranceMask |= InsuranceType::All;
            }
        }


        std::list<Insurance::InsuranceType> insuranceTypes = Insurance::GetInsuranceTypesFromMask(insuranceMask);

        // Check InsuranceTypes
        bool isAmmoInsured = false;
        for (const auto& type : insuranceTypes) {
            if (type == Insurance::InsuranceType::All) {
                isAmmoInsured = true;
                break;
            }
            if (type == Insurance::InsuranceType::Mines && aType == Archetype::MINE) {
                isAmmoInsured = true;
                break;
            }
            if (type == Insurance::InsuranceType::Projectiles && aType == Archetype::MUNITION) {
                isAmmoInsured = true;
                break;
            }
            if (type == Insurance::InsuranceType::Countermeasures && aType == Archetype::COUNTER_MEASURE) {
                isAmmoInsured = true;
                break;
            }
            if (type == Insurance::InsuranceType::ShieldBatteries && aType == Archetype::SHIELD_BATTERY) {
                isAmmoInsured = true;
                break;
            }
            if (type == Insurance::InsuranceType::Nanobots && aType == Archetype::REPAIR_KIT) {
                isAmmoInsured = true;
                break;
            }
        }

        if (isAmmoInsured)
            return true;

        return false;
    }

    bool isInsurableClass(Archetype::AClassType aType, uint iClientID)
    {
        // Charfilename
        CAccount* acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        std::string scUserFile = scAcctPath + wstos(wscAccDir) + Globals::FLHOOKUSER_FILE;

        std::wstring wscCharFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFilename);
        std::string scCharFilename = wstos(wscCharFilename);

        // Lese die INI-Werte
        bool bMines = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Mines", "OFF") == "ON");
        bool bProjectiles = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Projectiles", "OFF") == "ON");
        bool bCountermeasures = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Countermeasures", "OFF") == "ON");
        bool bShieldBatteries = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "ShieldBatteries", "OFF") == "ON");
        bool bNanobots = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Nanobots", "OFF") == "ON");
        bool bEquipment = (IniGetS(scUserFile, "INSURANCE-" + scCharFilename, "Equipment", "OFF") == "ON");

        // Berechne die insuranceMask
        InsuranceType insuranceMask = InsuranceType::None;

        if (bMines)
            insuranceMask |= InsuranceType::Mines;
        if (bProjectiles)
            insuranceMask |= InsuranceType::Projectiles;
        if (bCountermeasures)
            insuranceMask |= InsuranceType::Countermeasures;
        if (bShieldBatteries)
            insuranceMask |= InsuranceType::ShieldBatteries;
        if (bNanobots)
            insuranceMask |= InsuranceType::Nanobots;
        if (bEquipment)
            insuranceMask |= InsuranceType::Equipment;

        // Pr�fen, ob alle sechs Typen festgelegt sind
        if (bool allTypesSet = (bMines && bProjectiles && bCountermeasures && bShieldBatteries && bNanobots && bEquipment))
            insuranceMask |= InsuranceType::All;

        std::list<Insurance::InsuranceType> insuranceTypes = Insurance::GetInsuranceTypesFromMask(insuranceMask);

        // Check InsuranceTypes
        bool isEquipInsured = false;
        for (const auto& type : insuranceTypes) {
            if (type == Insurance::InsuranceType::All) {
                isEquipInsured = true;
                break;
            }
            if (type == Insurance::InsuranceType::Equipment) {
                isEquipInsured = true;
                break;
            }
        }

        // Check if equip is insured
        if (!isEquipInsured)
            return false;

        return aType == Archetype::SHIELD_GENERATOR ||
            aType == Archetype::THRUSTER ||
            aType == Archetype::LAUNCHER ||
            aType == Archetype::GUN ||
            aType == Archetype::MINE_DROPPER ||
            aType == Archetype::COUNTER_MEASURE_DROPPER ||
            aType == Archetype::CLOAKING_DEVICE;
    }

    std::string GetInsuranceTypeString(InsuranceType type) {
        switch (type) {
        case InsuranceType::None: return "None";
        case InsuranceType::Mines: return "Mines";
        case InsuranceType::Projectiles: return "Projectiles";
        case InsuranceType::Countermeasures: return "Countermeasures";
        case InsuranceType::ShieldBatteries: return "ShieldBatteries";
        case InsuranceType::Nanobots: return "Nanobots";
        case InsuranceType::Equipment: return "Equipment";
        case InsuranceType::All: return "All";
        default: return "";
        }
    }

    void SetInsuranceTypeString(uint iClientID, InsuranceType type, const std::string& value) {
        CAccount* acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        std::string scUserFile = scAcctPath + wstos(wscAccDir) + Globals::FLHOOKUSER_FILE;

        std::wstring wscFilename;
        HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
        std::string scFilename = wstos(wscFilename);

        std::string typeString = GetInsuranceTypeString(type);
        if (!typeString.empty()) {
            IniWrite(scUserFile, scFilename, "INSURANCE-" + typeString, value);
        }
    }

    Insurance::InsuranceType GetInsuranceTypeFromString(const std::string& value) {
        using enum Insurance::InsuranceType;

        std::string lowerValue = ToLower(value);

        if (lowerValue == "none" || lowerValue == "off") {
            return None;
        }
        else if (lowerValue == "mines" || lowerValue == "mine" || lowerValue == "m") {
            return Mines;
        }
        else if (lowerValue == "projectiles" || lowerValue == "pj" || lowerValue == "p" ||
            lowerValue == "rocket" || lowerValue == "rockets" || lowerValue == "torpedo" ||
            lowerValue == "torp" || lowerValue == "torps" || lowerValue == "missiles" ||
            lowerValue == "missile") {
            return Projectiles;
        }
        else if (lowerValue == "countermeasures" || lowerValue == "cm" || lowerValue == "cms") {
            return Countermeasures;
        }
        else if (lowerValue == "shieldbatteries" || lowerValue == "shield" || lowerValue == "shieldbats" ||
            lowerValue == "bats" || lowerValue == "sb" || lowerValue == "s") {
            return ShieldBatteries;
        }
        else if (lowerValue == "nanobots" || lowerValue == "bots" || lowerValue == "nanos" ||
            lowerValue == "nb" || lowerValue == "n") {
            return Nanobots;
        }
        else if (lowerValue == "equipment" || lowerValue == "equip" || lowerValue == "e") {
            return Equipment;
        }
        else if (lowerValue == "all") {
            return All;
        }

        return Invalid;
    }


    std::list<Insurance::InsuranceType> GetInsuranceTypesFromMask(Insurance::InsuranceType insuranceMask)
    {
        std::list<Insurance::InsuranceType> insuranceTypes;

        if ((insuranceMask & Insurance::InsuranceType::None) == Insurance::InsuranceType::None)
            insuranceTypes.push_back(Insurance::InsuranceType::None);
        if ((insuranceMask & Insurance::InsuranceType::Mines) == Insurance::InsuranceType::Mines)
            insuranceTypes.push_back(Insurance::InsuranceType::Mines);
        if ((insuranceMask & Insurance::InsuranceType::Projectiles) == Insurance::InsuranceType::Projectiles)
            insuranceTypes.push_back(Insurance::InsuranceType::Projectiles);
        if ((insuranceMask & Insurance::InsuranceType::Countermeasures) == Insurance::InsuranceType::Countermeasures)
            insuranceTypes.push_back(Insurance::InsuranceType::Countermeasures);
        if ((insuranceMask & Insurance::InsuranceType::ShieldBatteries) == Insurance::InsuranceType::ShieldBatteries)
            insuranceTypes.push_back(Insurance::InsuranceType::ShieldBatteries);
        if ((insuranceMask & Insurance::InsuranceType::Nanobots) == Insurance::InsuranceType::Nanobots)
            insuranceTypes.push_back(Insurance::InsuranceType::Nanobots);
        if ((insuranceMask & Insurance::InsuranceType::Equipment) == Insurance::InsuranceType::Equipment)
            insuranceTypes.push_back(Insurance::InsuranceType::Equipment);
        if ((insuranceMask & Insurance::InsuranceType::All) == Insurance::InsuranceType::All)
            insuranceTypes.push_back(Insurance::InsuranceType::All);

        return insuranceTypes;
    }

    bool CalcRemHold(uint iClientID)
    {
        //Player cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

        float CargofVolume = 0.0f;

        for (auto const& cargo : lstCargo) {
            Archetype::Equipment const* eq = Archetype::GetEquipment(cargo.iArchID);

            float itemVolume = eq->fVolume * static_cast<float>(cargo.iCount);

            CargofVolume += itemVolume;
        }

        uint iShipArchIDPlayer;
        pub::Player::GetShipID(iClientID, iShipArchIDPlayer);

        Archetype::Ship const* ship = Archetype::GetShip(iShipArchIDPlayer);
        float fSize = ship->fHoldSize;


        if (CargofVolume <= fSize) {
            ClientController::Send_ControlMsg(false, iClientID, L"allowundock 1\n");
            //ConPrint(L"allowundock 1 \n");
            return true;

        }
        else {
            ClientController::Send_ControlMsg(false, iClientID, L"allowundock 0\n");
            //ConPrint(L"allowundock 0 \n");
            return false;

        }

    }

}