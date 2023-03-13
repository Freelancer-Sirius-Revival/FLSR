#include "main.h"

namespace Depot {
	
	std::list<PlayerDepot> lPlayerDepot;
	
	void LoadDepotData() {
        std::string query;

        //Read Depot-List
        try
        {
            query = R"(SELECT * FROM "FLSR"."PlayerDepot";)";
            pqxx::result rPlayerDepot = SQL::CommitQuery(query);
			
            for (auto row : rPlayerDepot)
            {
				PlayerDepot NewDepot;
				NewDepot.iDepotID = row["id"].as<int>();
                NewDepot.iBaseID = row["baseid"].as<uint>();
                NewDepot.scAccountName = row["account_name"].as<std::string>();
                NewDepot.iCapacity = row["capacity"].as<int>();
				lPlayerDepot.push_back(NewDepot);
            }
			
        }
        catch (const std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
	}
	
    std::list<PlayerDepotItem> GetEquipFromBaseDepot(uint iClientID, bool bPrint)
    {
		//List of Equip
		std::list<PlayerDepotItem> lPlayerDepotItem;

		//Get Depot From Accountname
        CAccount* acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
		std::string scAccountName = wstos(wscAccDir);
        uint iBase = 0;
        pub::Player::GetBase(iClientID, iBase);
        std::string query;
        uint iDepotID;
        //GetPlayer Depot on base
        try
        {
            query = R"(SELECT * FROM "FLSR"."PlayerDepot" WHERE  "account_name" = ')" + scAccountName + R"(' AND "baseid" = ')" + std::to_string(iBase) + R"(';)";
            pqxx::result rPlayerDepot = SQL::CommitQuery(query);
			if (rPlayerDepot.size() == 0)
			{
				if (bPrint)
                    PrintUserCmdText(iClientID, L"You dont have a Depot on this Base!");
                return lPlayerDepotItem;
            }
            else {
                iDepotID = rPlayerDepot[0]["id"].as<int>();
                if (bPrint)
                    PrintUserCmdText(iClientID, L"DepotID: %u", iDepotID);
            }
			
        }
        catch (const std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
            return lPlayerDepotItem;
        }

        //Get Equip in Depot
        try
        {
            query = R"(SELECT * FROM "FLSR"."DepotStore" WHERE  "playerdepot" = ')" + std::to_string(iDepotID) + R"(';)";
            pqxx::result rDepotStore = SQL::CommitQuery(query);
            if (rDepotStore.size() == 0)
            {
                if (bPrint)
                    PrintUserCmdText(iClientID, L"You dont have any stored Equipment!");
                return lPlayerDepotItem;
            }
            else {
                for (auto row : rDepotStore)
                {
                    std::wstring wscEquipNick = stows(GetEquipNicknameFromID(row["goodid"].as<uint>()));
                    std::wstring wscAmount = std::to_wstring(row["amount"].as<int>());
                    if (bPrint)
                        PrintUserCmdText(iClientID, L"Equipment: " + wscEquipNick + L" Amount: " + wscAmount);

					PlayerDepotItem NewDepotItem;
					NewDepotItem.iDepotID = row["playerdepot"].as<int>();
					NewDepotItem.iGoodID = row["goodid"].as<uint>();
					NewDepotItem.iAmount = row["amount"].as<int>();
					lPlayerDepotItem.push_back(NewDepotItem);
                }
                
                //Return List
                return lPlayerDepotItem;
            }

        }
        catch (const std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
            return lPlayerDepotItem;
        }
        
    }

    void PlayerDepotOpen(uint iClientID)
    {
		//GetEquip in Depot
        std::list<PlayerDepotItem> lPlayerDepotItem = GetEquipFromBaseDepot(iClientID, false);

        //Prepare Data
        std::wstring wscCCMessage = L"DEPOTDATA={";

        //While List of Equip
        std::list<PlayerDepotItem>::iterator iterPlayerDepotItem = lPlayerDepotItem.begin();
        while (iterPlayerDepotItem != lPlayerDepotItem.end()) {
			//Get Equip Data
            std::wstring wscGoodID = std::to_wstring(iterPlayerDepotItem->iGoodID);
			std::wstring wscAmount = std::to_wstring(iterPlayerDepotItem->iAmount);
            
            //Set Data
            wscCCMessage = wscCCMessage + wscGoodID + L"-" + wscAmount;
            

            //Get last item of List
			if (iterPlayerDepotItem != --lPlayerDepotItem.end())
            {
                wscCCMessage = wscCCMessage + L",";
            }
            
            iterPlayerDepotItem++;
        }

		wscCCMessage = wscCCMessage + L"}";

		//Send to Client
        ConPrint(wscCCMessage + L"\n");
        ClientController::Send_ControlMsg(false, iClientID, wscCCMessage); 
        GetPlayerEquip(iClientID);
    }

    void GetPlayerEquip(uint iClientID)
    {
		//Create New List of Equip
		std::list<PlayerCargoItem> lPlayerCargoItem;

		//Get Player Equip
        std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);

        std::list<CARGO_INFO> lstEquipment;
        int iRemaining;
        HkEnumCargo(wscCharname, lstEquipment, iRemaining);

		//Create List of Equip
        std::list<CARGO_INFO> lstMounted;
        for (auto& cargo : lstEquipment) {       
			if (!cargo.bMounted && cargo.fStatus == 1.0f) 
            {
                //Get only Equipment and commodity_credits
                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi) {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi) {
						if (gi->iType != 0 || GetEquipNicknameFromID(cargo.iArchID) == "commodity_credits") {
                            //New PlayerCargoItem
							PlayerCargoItem NewPlayerCargoItem;
							NewPlayerCargoItem.iGoodID = cargo.iArchID;
							NewPlayerCargoItem.iAmount = cargo.iCount;
							lPlayerCargoItem.push_back(NewPlayerCargoItem);   

                            //Debug
							//std::wstring wscGoodID = std::to_wstring(cargo.iArchID);
							//std::wstring wscAmount = std::to_wstring(cargo.iCount);
							//ConPrint(L"GoodID: " + wscGoodID + L" Amount: " + wscAmount + L"\n");
                        }
                    }
                }
            }
        }

        //Send List to Client
        //Prepare Data
        std::wstring wscCCMessage = L"CARGODATA={";
       
        //While List of Equip
        std::list<PlayerCargoItem>::iterator iterPlayerCargoItem = lPlayerCargoItem.begin();
        while (iterPlayerCargoItem != lPlayerCargoItem.end()) {
            //Get Equip Data
            std::wstring wscGoodID = std::to_wstring(iterPlayerCargoItem->iGoodID);
            std::wstring wscAmount = std::to_wstring(iterPlayerCargoItem->iAmount);

            //Set Data
            wscCCMessage = wscCCMessage + wscGoodID + L"-" + wscAmount;


            //Get last item of List
            if (iterPlayerCargoItem != --lPlayerCargoItem.end())
            {
                wscCCMessage = wscCCMessage + L",";
            }

            iterPlayerCargoItem++;
        }

        wscCCMessage = wscCCMessage + L"}";

        //Send to Client
        ConPrint(wscCCMessage + L"\n");
        ClientController::Send_ControlMsg(false, iClientID, wscCCMessage);
        
    }
	
    std::string GetEquipNicknameFromID(uint goodID) {
    
        Tools::HashMap NewHash = Tools::mNicknameHashMap[goodID];
        return NewHash.scNickname;

       // Archetype::Ship* ship = Archetype::GetShip(2820316814);
       // std::wstring test = HkGetWStringFromI  b  b  DS(459545).c_str();

        //ConPrint(test+L"\n");		
    }



}