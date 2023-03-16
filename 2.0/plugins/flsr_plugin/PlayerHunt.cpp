#include "Main.h"

namespace PlayerHunt {

	float set_fPlayerHuntMulti;
	float set_fPlayerHuntTick;
	int set_iPlayerHuntMinSystems = 4;
	std::vector<std::string> SystemWhitelist = {
	"Hi01", "Li05", "Li01", "Li03", "Rh05", "Rh01", "Rh03", "Br06", "Br04", "Br02",
	"Ku06", "Ku04", "Ku02", "Iw06", "Iw04", "Iw02", "Bw10", "Bw08", "Bw06", "Bw04",
	"Bw02", "Ew04", "Ew02", "Hi02", "Li04", "Li02", "Rh04", "Rh02", "Br05", "Br03",
	"Br01", "Ku07", "Ku05", "Ku03", "Ku01", "Iw05", "Iw03", "Iw01", "Bw11", "Bw09",
	"Bw07", "Bw05", "Bw03", "Bw01", "Ew03", "Ew01"
	};
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
			//Check Whitelist
			bool bWhitelisted = false;
			for (std::vector<std::string>::iterator t = SystemWhitelist.begin(); t != SystemWhitelist.end(); ++t)
			{
				std::string scSystemNickname = sysinfo->nickname;
				if (scSystemNickname.find(*t) != std::string::npos) {
					//ConPrint(stows(scSystemNickname) + L" is whitelisted\n");
					bWhitelisted = true;
					continue;
				}
				
			}

			if (bWhitelisted)
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

	BaseData getRandomBaseInSystem(uint iPlayerSystemID, uint iClientID)
	{
		//Get Factions fpr Reputation Check
		std::list<Tools::RepCB> lstTagFactions = Tools::HkGetFactions();

		//Get all bases of System
		std::vector<BaseData> bases;
		//ConPrint(L"loopstart\n");

		for (auto& baseinfo : lstBases) {
			
			// Check if base is in system
			if (baseinfo.iSystemID == iPlayerSystemID)
			{
				// Check if player has access to base


				BASE_INFO bi;
				bi.bDestroyed = false;
				bi.iObjectID = baseinfo.iObjectID;
				bi.scBasename = baseinfo.scBasename;
				bi.iBaseID = CreateID(baseinfo.scBasename.c_str());
				
				// get base rep
				int iSolarRep;
				pub::SpaceObj::GetSolarRep(bi.iObjectID, iSolarRep);
				uint iBaseRep;
				pub::Reputation::GetAffiliation(iSolarRep, iBaseRep);
				// rep can't be determined yet(space object not created yet?)
				if (iBaseRep != -1)
				{
					// get player rep
					int iRepID;
					pub::Player::GetRep(iClientID, iRepID);

					// check if rep is sufficient
					float fPlayerRep;
					pub::Reputation::GetGroupFeelingsTowards(iRepID, iBaseRep, fPlayerRep);

					//Check if Player has access to base (-0,4 min rep)
					if (fPlayerRep >= -0.4f)
					{
						std::string scBasename = bi.scBasename;
						std::string scLowerBase = ToLower(scBasename);



						if (scLowerBase.find("base") != std::string::npos)
						{
							BaseData NewBase;
							NewBase.iBaseID = baseinfo.iBaseID;
							NewBase.iSystemID = baseinfo.iSystemID;
							NewBase.scBaseNickname = bi.scBasename;


							bases.push_back(NewBase);
							//ConPrint(L"Base added to list:" + stows(scBasename) + L"\n");


						}
					}
				}
				else {
					// rep can't be determined yet(space object not created yet?)
					//ConPrint(L"rep can't be determined yet(space object not created yet?\n");
				}
			}
		}
		//ConPrint(L"loopend\n");

		if (bases.size() == 0)
		{
			ConPrint(L"No bases found\n");
			return BaseData();
		}
		else
		{
			//Get random base
			int random = rand() % bases.size();
			return bases[random];
		}
	}

	BaseData getTargetBase(uint iClientID)
	{
		BaseData TargetBase;

		while (TargetBase.scBaseNickname == "")
		{
			//Get random system in range
			uint iSysID = getRandomSysteminRange(iClientID);

			//Get random base in system
			BaseData tempBase = getRandomBaseInSystem(iSysID, iClientID);

			//Check if TargetBase has data
			if (tempBase.scBaseNickname != "")
			{
				TargetBase = tempBase;
			}
		}
	
		ConPrint(stows(TargetBase.scBaseNickname) + L" done\n");
		
		return TargetBase;
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