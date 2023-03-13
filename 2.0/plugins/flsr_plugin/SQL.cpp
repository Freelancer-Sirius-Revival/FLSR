#include "main.h"

namespace SQL {

    bool InitializeedServerData = false;


    void InitializePlayerDB() {

        uint iMaxPlayer = MAX_CLIENT_ID;
        std::string query;

        //Empty old DB
        try
        {
            query = R"(TRUNCATE TABLE "FLSR"."PlayerOnline";)";
            pqxx::result PlayerOnline = CommitQuery(query);
        
            query = "";
            pqxx::connection connection = Connect();
            pqxx::work worker(connection);

            for (size_t i = 1; i < iMaxPlayer; i++)
            {
            
                query = query + R"(INSERT INTO "FLSR"."PlayerOnline" ("id") VALUES(')" + std::to_string(i) + R"(');)";

            }
		
            pqxx::result result = worker.exec(query.c_str());
            worker.commit();
        }
        catch (const std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error));
        }
    }

    void InitializeServerData() {

        if (!InitializeedServerData)
        {
			InitializeedServerData = true;
			
            std::string query;

			//Update ServerData
            try
            {
				//Get Player Online
				int iPlayerOnline = 0;
                struct PlayerData* pPD = 0;
                while (pPD = Players.traverse_active(pPD)) {
                    iPlayerOnline++;
                }
				
				//Update Player Online
                query = R"(UPDATE "FLSR"."Data" SET "Data"=')" + std::to_string(iPlayerOnline) + R"(' WHERE  "id"=1;)";
                pqxx::result PlayerOnline = CommitQuery(query);

                //Update Max Player
                query = R"(UPDATE "FLSR"."Data" SET "Data"=')" + std::to_string(Players.GetMaxPlayerCount()) + R"(' WHERE  "id"=2;)";
                pqxx::result MaxPlayer = CommitQuery(query);
            }
            catch (const std::exception& e)
            {
                std::string error = e.what();
                ConPrint(L"SQLERROR: " + stows(error));
            }

			
        }
       
    }

    void Timer2000ms() {
        if (Modules::GetModuleState("SQLModule"))
        {
            InitializeServerData();
			
            HANDLE Thread;
            DWORD id;
            DWORD dwParam;
            Thread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Thread2000ms, &dwParam, 0, &id);
			
           
        }
    }

    void Thread2000ms(){

       // Depot::GetEquipname(3024562509);
		
        int iPlayers = 0;
        std::string query;
        try
        {
 
            //Get Data from DATA Store
            query = "SELECT * FROM  \"FLSR\".\"Data\"";
            pqxx::result FLSRData = CommitQuery(query);

            //Parse Data
            std::string sOnlinePlayers = FLSRData[0][1].as<std::string>();
            std::string sMaxPlayers = FLSRData[1][1].as<std::string>();
			
            //Get Player Online
            int iPlayerOnline = 0;
            struct PlayerData* pPD = 0;
            while (pPD = Players.traverse_active(pPD)) {
                uint iClientID = pPD->iOnlineID;
                if (!HkIsInCharSelectMenu(iClientID)) {
                    // Update PlayerList
                    iClientID = HkGetClientIdFromPD(pPD);
                    const wchar_t* wszCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
                    int iCash;
                    HkGetCash(ARG_CLIENTID(iClientID), iCash);
                    uint iShipArchIDPlayer;
                    pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
                    uint iSystemID;
                    pub::Player::GetSystem(iClientID, iSystemID);
                    std::wstring wscSystemNickname = HkGetSystemNickByID(iSystemID);
                    uint iBaseID = ClientInfo[iClientID].iLastExitedBaseID;
                    std::wstring wscBaseNickname = HkGetBaseNickByID(iBaseID);

                   
                    //std::string scShipNickname; No Data yet

                    query = R"(UPDATE "FLSR"."PlayerOnline" SET "Charname"=')" + wstos(wszCharname) + R"(', "Credits"=')" + std::to_string(iCash) + R"(', "Ship"=')" + std::to_string(iShipArchIDPlayer) + R"(', "System"=')" + wstos(wscSystemNickname) + R"(', "LastBase"=')" + wstos(wscBaseNickname) + R"(' WHERE  "id"=)" + std::to_string(iClientID) + R"(;)";
                    pqxx::result PlayerData = CommitQuery(query);
                }
                iPlayerOnline++;

            }

			//Update Player Online
            if (iPlayerOnline != std::stoi(sOnlinePlayers))
            {
				//Update Player Online
				query = R"(UPDATE "FLSR"."Data" SET "Data"=')" + std::to_string(iPlayerOnline) + R"(' WHERE  "id"=1;)";
				pqxx::result PlayerOnline = CommitQuery(query);
            }


        }
        catch (const std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error));
        }

		
    }
	
	
    pqxx::connection Connect()
    {
        std::string connectionString = SQLDATA;
        pqxx::connection connectionObject(connectionString.c_str());
        return connectionObject;
    }
	
    pqxx::result CommitQuery(std::string query)
    {
        std::string connectionString = SQLDATA;
        pqxx::connection connectionObject(connectionString.c_str());
        pqxx::work worker(connectionObject);
        pqxx::result result = worker.exec(query.c_str());
        worker.commit();
        return result;
    }

}