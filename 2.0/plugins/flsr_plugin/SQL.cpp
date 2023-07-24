#include "main.h"

namespace SQL {

	std::string scDbName;
	
	void InitializeDB()
	{
		//FLPath
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		scDbName = std::string(szCurDir) + "\\flhook_plugins\\flsr.db3";

		SQLite::Database db(SQL::scDbName, SQLITE_OPEN_READWRITE);

        CheckAndCreateDuelRankingTable();
        CheckAndCreateFFARankingTable();
        CheckAndCreatePVPRankingTable();
        CheckAndCreatePVERankingTable();

		
	}

    void CheckAndCreateDuelRankingTable()
    {
        try
        {
            SQLite::Database db(SQL::scDbName, SQLITE_OPEN_READWRITE);

            // Überprüfen, ob die Tabelle "DuelRanking" existiert
            SQLite::Statement queryCheckTable(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='DuelRanking'");
            queryCheckTable.executeStep();
            int tableExists = queryCheckTable.getColumn(0).getInt();

            if (!tableExists)
            {
                // Tabelle "DuelRanking" erstellen mit UTF-8 Collation
                db.exec("CREATE TABLE DuelRanking (ID INTEGER PRIMARY KEY AUTOINCREMENT, Charname TEXT COLLATE NOCASE, Kills INTEGER, Deaths INTEGER, Charfile TEXT COLLATE NOCASE)");
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }

    void CheckAndCreateFFARankingTable()
    {
        try
        {
            SQLite::Database db(SQL::scDbName, SQLITE_OPEN_READWRITE);

            // Überprüfen, ob die Tabelle "DuelRanking" existiert
            SQLite::Statement queryCheckTable(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='FFARanking'");
            queryCheckTable.executeStep();
            int tableExists = queryCheckTable.getColumn(0).getInt();

            if (!tableExists)
            {
                // Tabelle "DuelRanking" erstellen mit UTF-8 Collation
                db.exec("CREATE TABLE FFARanking (ID INTEGER PRIMARY KEY AUTOINCREMENT, Charname TEXT COLLATE NOCASE, Kills INTEGER, Deaths INTEGER, Charfile TEXT COLLATE NOCASE)");
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }

    void CheckAndCreatePVPRankingTable()
    {
        try
        {
            SQLite::Database db(SQL::scDbName, SQLITE_OPEN_READWRITE);

            // Überprüfen, ob die Tabelle "DuelRanking" existiert
            SQLite::Statement queryCheckTable(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='PVPRanking'");
            queryCheckTable.executeStep();
            int tableExists = queryCheckTable.getColumn(0).getInt();

            if (!tableExists)
            {
                // Tabelle "DuelRanking" erstellen mit UTF-8 Collation
                db.exec("CREATE TABLE PVPRanking (ID INTEGER PRIMARY KEY AUTOINCREMENT, Charname TEXT COLLATE NOCASE, Kills INTEGER, Deaths INTEGER, Charfile TEXT COLLATE NOCASE)");
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }

    void CheckAndCreatePVERankingTable()
    {
        try
        {
            SQLite::Database db(SQL::scDbName, SQLITE_OPEN_READWRITE);

            // Überprüfen, ob die Tabelle "DuelRanking" existiert
            SQLite::Statement queryCheckTable(db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='PVERanking'");
            queryCheckTable.executeStep();
            int tableExists = queryCheckTable.getColumn(0).getInt();

            if (!tableExists)
            {
                // Tabelle "DuelRanking" erstellen mit UTF-8 Collation
                db.exec("CREATE TABLE PVERanking (ID INTEGER PRIMARY KEY AUTOINCREMENT, Charname TEXT COLLATE NOCASE, Kills INTEGER, Deaths INTEGER, Charfile TEXT COLLATE NOCASE)");
            }
        }
        catch (std::exception& e)
        {
            std::string error = e.what();
            ConPrint(L"SQLERROR: " + stows(error) + L"\n");
        }
    }

}