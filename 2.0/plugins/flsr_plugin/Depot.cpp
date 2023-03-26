#include "main.h"

namespace Depot {
	
	std::list<PlayerDepot> lPlayerDepot;

    bool LoadDepotData() {
        //Read Depot-List
        try
        {
            // Open a database file
            SQLite::Database db(SQL::scDbName);

            // Compile a SQL query, containing one parameter (index 1)
            SQLite::Statement query(db, R"(SELECT * FROM "PlayerDepot";)");


            // Loop to execute the query step by step, to get rows of result
            while (query.executeStep())
            {


                PlayerDepot NewDepot;
                NewDepot.iDepotID = query.getColumn(0);
                NewDepot.iBaseID = query.getColumn(1);
                const char* szAccountName = query.getColumn(2);
                std::string scAccountName = szAccountName;
                NewDepot.scAccountName = scAccountName;
                NewDepot.iCapacity = query.getColumn(3);
                lPlayerDepot.push_back(NewDepot);
            }
            
            return true;
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
            return false;
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
        uint iDepotID;
        //GetPlayer Depot on base
        try
        {
            // Open a database file
            SQLite::Database db(SQL::scDbName);

            // Compile a SQL query, containing one parameter (index 1)
            SQLite::Statement query(db, R"(SELECT * FROM "PlayerDepot" WHERE  "AccountName" = ')" + scAccountName + R"(' AND "BaseID" = ')" + std::to_string(iBase) + R"(';)");
            
            ConPrint(stows(R"(SELECT * FROM "PlayerDepot" WHERE  "AccountName" = ')" + scAccountName + R"(' AND "BaseID" = ')" + std::to_string(iBase) + R"(';)") + L"\n");

            // Loop to execute the query step by step, to get rows of result
            query.executeStep();
            
            //Check Results
            if (!query.hasRow())
            {
                if (bPrint)
                    PrintUserCmdText(iClientID, L"You dont have a Depot on this Base!");
                return lPlayerDepotItem;
            }
            else {
               iDepotID = query.getColumn(0);
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
            // Open a database file
            SQLite::Database db(SQL::scDbName);

            // Compile a SQL query, containing one parameter (index 1)
            SQLite::Statement query(db, R"(SELECT * FROM "DepotStore" WHERE  "PlayerDepot" = ')" + std::to_string(iDepotID) + R"(';)");
            
            ConPrint(stows(R"(SELECT * FROM "DepotStore" WHERE  "PlayerDepot" = ')" + std::to_string(iDepotID) + R"(';)") + L"\n");
            
            bool bEquipfound = false;
            

            // Loop to execute the query step by step, to get rows of result
            while (query.executeStep())
			{

                uint iArchID = query.getColumn(2);
				uint iAmount = query.getColumn(3);
                
                std::wstring wscEquipNick = stows(GetEquipNicknameFromID(iArchID));
                std::wstring wscAmount = std::to_wstring(iAmount);
                
                if (bPrint)
                    PrintUserCmdText(iClientID, L"Equipment: " + wscEquipNick + L" Amount: " + wscAmount);


                PlayerDepotItem NewDepotItem;
                NewDepotItem.iDepotID = iDepotID;
                NewDepotItem.iGoodID = iArchID;
                NewDepotItem.iAmount = iAmount;
                
                //Get only Equipment/Goods
                const GoodInfo* gi = GoodList_get()->find_by_id(iArchID);
                if (gi)
                {
                    NewDepotItem.iIDSName = gi->iIDSName;

                }
                else {
                    // Check Archtype
                    Archetype::Equipment* eq = Archetype::GetEquipment(iArchID);
                    NewDepotItem.iIDSName = eq->iIdsName;
                }                
                
                lPlayerDepotItem.push_back(NewDepotItem);   

                bEquipfound = true;
            }
            
            //Check Results
            if (!bEquipfound)
            {
                if (bPrint)
                    PrintUserCmdText(iClientID, L"You dont have any stored Equipment!");
                return lPlayerDepotItem;
            }
            else {
                PrintUserCmdText(iClientID, L"Equip found!");
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
        std::list<PlayerDepotItem> lPlayerDepotItem = GetEquipFromBaseDepot(iClientID, true);

        //Prepare Data
        std::wstring wscCCMessage = L"DEPOTDATA={";

        //While List of Equip
        std::list<PlayerDepotItem>::iterator iterPlayerDepotItem = lPlayerDepotItem.begin();
        while (iterPlayerDepotItem != lPlayerDepotItem.end()) {
            //Get Equip Data
            std::wstring wscIDSName = std::to_wstring(iterPlayerDepotItem->iIDSName);
            std::wstring wscGoodID = std::to_wstring(iterPlayerDepotItem->iGoodID);
            std::wstring wscAmount = std::to_wstring(iterPlayerDepotItem->iAmount);

            //Set Data
            wscCCMessage = wscCCMessage + wscIDSName + L"-" + wscGoodID + L"-" + wscAmount;


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
                //New PlayerCargoItem
                PlayerCargoItem NewPlayerCargoItem;
                NewPlayerCargoItem.iGoodID = cargo.iArchID;
                NewPlayerCargoItem.iAmount = cargo.iCount;

                //Get only Equipment/Goods
                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi)
                {
                    NewPlayerCargoItem.iIDSName = gi->iIDSName;

                }
                else {
                    // Check Archtype
                    Archetype::Equipment* eq = Archetype::GetEquipment(cargo.iArchID);
                    NewPlayerCargoItem.iIDSName = eq->iIdsName;
                }

                lPlayerCargoItem.push_back(NewPlayerCargoItem);
            }
        }

        //Send List to Client
        //Prepare Data
        std::wstring wscCCMessage = L"CARGODATA={";

        //While List of Equip
        std::list<PlayerCargoItem>::iterator iterPlayerCargoItem = lPlayerCargoItem.begin();
        while (iterPlayerCargoItem != lPlayerCargoItem.end()) {
            //Get Equip Data
			std::wstring wscIDSName = std::to_wstring(iterPlayerCargoItem->iIDSName);
            std::wstring wscGoodID = std::to_wstring(iterPlayerCargoItem->iGoodID);
            std::wstring wscAmount = std::to_wstring(iterPlayerCargoItem->iAmount);

            //Set Data
            wscCCMessage = wscCCMessage + wscIDSName + L"-" + wscGoodID + L"-" + wscAmount;


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
    
    //Not tested:

    void StoreEquipInDatabase(uint iClientID, uint goodID)
    {
        // Überprüfe, ob der Spieler das angegebene Equip im Cargo hat
        bool bHasEquip = false;
        std::list<CARGO_INFO> lstCargo;
        int iRemaining;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemaining);
        for (auto& cargo : lstCargo)
        {
            if (cargo.iArchID == goodID)
            {
                bHasEquip = true;
                break;
            }
        }

        if (!bHasEquip)
        {
            PrintUserCmdText(iClientID, L"Du hast dieses Equip nicht im Cargo!");
            return;
        }

        // Finde das Depot des Spielers auf der Basis
        uint iBase = 0;
        pub::Player::GetBase(iClientID, iBase);
        CAccount* acc = Players.FindAccountFromClientID(iClientID);
        std::wstring wscAccDir;
        HkGetAccountDirName(acc, wscAccDir);
        std::string scAccountName = wstos(wscAccDir);
        uint iDepotID;

        try
        {
            // Öffne die Datenbankdatei
            SQLite::Database db(SQL::scDbName);

            // Führe eine SQL-Abfrage aus, um das Depot des Spielers auf der Basis zu finden
            SQLite::Statement query(db, R"(SELECT * FROM "PlayerDepot" WHERE  "AccountName" = ')" + scAccountName + R"(' AND "BaseID" = ')" + std::to_string(iBase) + R"(';)");
            query.executeStep();

            // Überprüfe die Ergebnisse
            if (!query.hasRow())
            {
                PrintUserCmdText(iClientID, L"Du hast kein Depot auf dieser Basis!");
                return;
            }
            else {
                iDepotID = query.getColumn(0);
                PrintUserCmdText(iClientID, L"DepotID: %u", iDepotID);
            }
        }
        catch (const std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
            return;
        }

        // Füge das Equip in das Depot ein
        try
        {
            // Öffne die Datenbankdatei
            SQLite::Database db(SQL::scDbName);

            // Führe eine SQL-Abfrage aus, um das Equip in das Depot einzufügen
            SQLite::Statement query(db, R"(INSERT INTO "PlayerDepotEquip" ("DepotID", "GoodID") VALUES (')" + std::to_string(iDepotID) + R"(', ')" + std::to_string(goodID) + R"(');)");
            query.executeStep();

            PrintUserCmdText(iClientID, L"Equip wurde erfolgreich eingelagert!");

            // Entferne das Equip aus dem Cargo des Spielers
            HkRemoveCargo(ARG_CLIENTID(iClientID), goodID, 1);
        }
        catch (const std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
            return;
        }
    }

}