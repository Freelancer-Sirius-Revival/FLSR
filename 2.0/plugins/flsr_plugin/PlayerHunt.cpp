#include "Main.h"

namespace PlayerHunt {

	float set_fPlayerHuntMulti;
	float set_fPlayerHuntTick;
	int set_iPlayerHuntMinSystems;
	ServerHuntInfo ServerHuntData;
	
	uint getRandomSysteminRange(uint iClientID)
	{
		//Get player system
		uint iSysIDPlayer;
		pub::Player::GetSystem(iClientID, iSysIDPlayer);
		
		//Get player system nickname
		char szSystemnamePlayer[1024] = "";
		pub::GetSystemNickname(szSystemnamePlayer, sizeof(szSystemnamePlayer), iSysIDPlayer);
		
		//Get all systems
		std::vector<std::string> systems;
		struct Universe::ISystem* sysinfo = Universe::GetFirstSystem();
		while (sysinfo) {
			systems.push_back(sysinfo->nickname);
			sysinfo = Universe::GetNextSystem();
		}

		//Get random system in min Range
		int range = 0;
		std::string randomSystem;
		while (range <= set_iPlayerHuntMinSystems) {
			int random = rand() % systems.size();
			randomSystem = systems[random];
			range = Tools::CountShortestPath(szSystemnamePlayer, randomSystem);

		}			
		return Universe::get_system_id(randomSystem.c_str());
	}

	uint getRandomBaseInSystem(uint iSystemID, uint iClientID)
	{
		//Get Factions fpr Reputation Check
		std::list<Tools::RepCB> lstTagFactions = Tools::HkGetFactions();




		//Get all bases of System
		std::vector<uint> bases;
		struct Universe::IBase* baseinfo = Universe::GetFirstBase();
		while (baseinfo) {
			// Check if base is in system
			if (baseinfo->iSystemID == iSystemID) {
				// Check if player has access to base


				BASE_INFO* bi = 0;
				for (auto& base : lstBases) {
					if (base.iBaseID == baseinfo->iBaseID) {
						bi = &base;
						break;
					}
				}
				
				// get base rep
				int iSolarRep;
				pub::SpaceObj::GetSolarRep(bi->iObjectID, iSolarRep);
				uint iBaseRep;
				pub::Reputation::GetAffiliation(iSolarRep, iBaseRep);
				if (iBaseRep == -1)
					continue; // rep can't be determined yet(space object not created yet?)

				// get player rep
				int iRepID;
				pub::Player::GetRep(iClientID, iRepID);

				// check if rep is sufficient
				float fPlayerRep;
				pub::Reputation::GetGroupFeelingsTowards(iRepID, iBaseRep, fPlayerRep);
				
				//Check if Player has access to base (-0,4 min rep)
				if (fPlayerRep >= -0.4f)
				{
					uint iBaseID = 0;
					std::wstring wscBasename = stows(bi->scBasename);

					//Check if base is Valid to Dock
					typedef int (*_GetString)(LPVOID, uint, wchar_t*, uint);
					_GetString GetString = (_GetString)0x4176b0;
					Universe::IBase* pBase = Universe::GetFirstBase();
					while (pBase) {
						wchar_t buf[1024];
						GetString(NULL, pBase->iBaseIDS, buf, 1024);
						if (wcsstr(buf, wscBasename.c_str())) {
							// Ignore the intro bases.
							if (_strnicmp("intro", (char*)pBase->iDunno2, 5) != 0) {
								iBaseID = pBase->iBaseID;
								break;
							}
						}
						pBase = Universe::GetNextBase();
					}

					ConPrint(L"Base to list:" + wscBasename + L"\n");

					
					if (iBaseID != 0)
					{
						bases.push_back(baseinfo->iBaseID);
						ConPrint(L"Base added to list:" + wscBasename + L"\n");
					}
				}

			}
			baseinfo = Universe::GetNextBase();
		}

		//Get random base
		int random = rand() % bases.size();
		return bases[random];
	}

	/*
	void Start_PlayerHunt(uint iClientID)
	{
		//Check state with switch case
		switch (ServerHuntData.eState) {
		case HUNT_STATE_NONE:
			//We can start a hunt
			
		case HUNT_STATE_DISCONNECTED:
			//We can capture the hunt
			
			//Get Base of Player
			uint iBaseIDPlayer;
			pub::Player::GetBase(iClientID, iBaseIDPlayer);
			
			//Check if player is docked
			if (iBaseIDPlayer == 0) {
				//Player is not in a base
				PrintUserCmdText(iClientID, L"ERR You are not in a base");
				return;
			}
			
			//Get new random system
			uint iNewSystem = getRandomSysteminRange(iClientID);
			
			//Get new random base
			uint iNewBase = getRandomBaseInSystem(iNewSystem, iClientID);
			
		case HUNT_STATE_WON:
			//We must wait for the next hunt (Timer)
			PrintUserCmdText(iClientID, L"ERR You must wait for the next hunt");
			return;
			
		case HUNT_STATE_HUNTING:
			//We must wait for the next hunt
			PrintUserCmdText(iClientID, L"ERR A Hunt is already active");
			return;

		}		
	}
	*/






}