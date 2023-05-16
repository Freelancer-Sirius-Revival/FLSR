#include "main.h"

namespace SQL {

	std::string scDbName;
	
	void InitializeDB()
	{
		//FLPath
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		scDbName = std::string(szCurDir) + "\\flhook_plugins\\flsr.db3";

		SQLite::Database db(SQL::scDbName, SQLite::OPEN_READWRITE);
		
	}
}