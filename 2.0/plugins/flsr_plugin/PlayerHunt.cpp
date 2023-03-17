#include "Main.h"

namespace PlayerHunt {

	float set_fRewardMultiplicator;
	int set_iMultiplicatorTriggerTime;
	int set_iMinCredits;
	int set_iMinTargetSystemDistance;

	std::vector<std::string> SystemWhitelist = {
	"Hi01", "Li05", "Li01", "Li03", "Rh05", "Rh01", "Rh03", "Br06", "Br04", "Br02",
	"Ku06", "Ku04", "Ku02", "Iw06", "Iw04", "Iw02", "Bw10", "Bw08", "Bw06", "Bw04",
	"Bw02", "Ew04", "Ew02", "Hi02", "Li04", "Li02", "Rh04", "Rh02", "Br05", "Br03",
	"Br01", "Ku07", "Ku05", "Ku03", "Ku01", "Iw05", "Iw03", "Iw01", "Bw11", "Bw09",
	"Bw07", "Bw05", "Bw03", "Bw01", "Ew03", "Ew01"
	};

	ServerHuntInfo ServerHuntData;

	void LoadPlayerHuntSettings()
	{
		// Konfigpfad
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		std::string scPluginCfgFile = std::string(szCurDir) + PLUGIN_CONFIG_FILE;

		set_fRewardMultiplicator = IniGetF(scPluginCfgFile, "PlayerHunt", "RewardMultiplicator", 0.5f);
		set_iMultiplicatorTriggerTime = IniGetI(scPluginCfgFile, "PlayerHunt", "MultiplicatorTriggerTime", 600);
		set_iMinTargetSystemDistance = IniGetI(scPluginCfgFile, "PlayerHunt", "MinTargetSystemDistance", 4);
		set_iMinCredits = IniGetI(scPluginCfgFile, "PlayerHunt", "MinCredits", 50000);



	}

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
		while (range <= set_iMinTargetSystemDistance) {
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
			
		return TargetBase;
	}

	void CalcReward()
	{
		int iCredits = ServerHuntData.iCredits;
		int iReward = iCredits + (iCredits * set_fRewardMultiplicator);
		ServerHuntData.iCredits = iReward;
	}
	
	void PlayerHuntTimer()
	{
		//Check for Modul
		if (Modules::GetModuleState("PlayerHunt"))
		{
			//Check if Hunt is active
			if (ServerHuntData.eState == HUNT_STATE_HUNTING)
			{
				uint now = timeInMS();
				uint iNextUpdate = ServerHuntData.tmHuntTime + (set_iMultiplicatorTriggerTime * 1000);

				if (now > iNextUpdate)
				{
					ServerHuntData.tmHuntTime = timeInMS();
					CalcReward();

					HkMsgU(L"The new PlayerHunt reward is " + std::to_wstring(ServerHuntData.iCredits) + L" credits!");
					HkMsgU(L"Hunt the player " + ServerHuntData.wscCharname + L" and get the reward!");
				}
			}
		}
	}
	
	void CheckSystemReached(uint iClientID, uint iPlayerSystemID)
	{
		//Check if Hunt is active
		if (ServerHuntData.eState == HUNT_STATE_HUNTING)
		{
			//Check is Player has a Hunt
			// Get the current character name
			std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
			if (ServerHuntData.wscCharname == wscCharname)
			{
				//We have the Hunt				
				//Check if we are in the TargetSystem
				if (ServerHuntData.iTargetSystem == iPlayerSystemID)
				{
					//We are now in the TargetSystem
					HkMsgU(ServerHuntData.wscCharname + L" has reached " + ServerHuntData.wscTargetSystem + L" kill the player before he docks at " + ServerHuntData.wscTargetBase);
					PrintUserCmdText(iClientID, L"You have reached the target system be careful players could be near!");					
				}
			}
		}
	}

	void CheckDock(uint iBaseID, uint iClientID)
	{
		//Check if Hunt is active
		if (ServerHuntData.eState == HUNT_STATE_HUNTING)
		{
			//Check is Player has a Hunt
			// Get the current character name
			std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
			if (ServerHuntData.wscCharname == wscCharname)
			{
				//We have the Hunt	
				
				//Check if we dock at our TargetBase
				if (ServerHuntData.iTargetBase == iBaseID)
				{

					//Win!
					// Add cash to target character
					HkAddCash(wscCharname, ServerHuntData.iCredits);

					//Print Announcement
					HkMsgU(ServerHuntData.wscCharname + L" has reached " + ServerHuntData.wscTargetBase + L"! The player has received a reward of " + std::to_wstring(ServerHuntData.iCredits) + L" credits!");
					PrintUserCmdText(iClientID, L"You have survived!");
					ServerHuntData.eState = HUNT_STATE_NONE;
				}
			}
		}
	}

	void CheckDisConnect(uint iClientID)
	{
		//Check if Hunt is active
		if (ServerHuntData.eState == HUNT_STATE_HUNTING)
		{
			//Check is Player has a Hunt
			// Get the current character name
			std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
			if (ServerHuntData.wscCharname == wscCharname)
			{
				//We have the Hunt
				HkMsgU(ServerHuntData.wscCharname + L" has disconnected! Dock at a base and take over the mission with /playerhunt!");
				ServerHuntData.eState = HUNT_STATE_DISCONNECTED;

			}
		}
	}

	void CheckDied(uint iClientID,uint iKillerID)
	{
		//No Killer
		if (iKillerID == 0)
			return;

		//Check if Hunt is active
		if (ServerHuntData.eState == HUNT_STATE_HUNTING)
		{
			//Check is Player has a Hunt
			// Get the current character name
			std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
			std::wstring wscCharnameNew = (const wchar_t*)Players.GetActiveCharacterName(iKillerID);

			if (ServerHuntData.wscCharname == wscCharname)
			{
				//We have the Hunt

				//Update the Hunt
				ServerHuntData.wscCharname = wscCharnameNew;

				HkMsgU(ServerHuntData.wscCharname + L" died! The hunted player is now " + wscCharnameNew + L"!");
				HkMsgU(L"Hunt the player to get the reward of " + std::to_wstring(ServerHuntData.iCredits) + L" Credits yourself!");
				PrintUserCmdText(iClientID, L"Reach " + ServerHuntData.wscTargetBase + L" in System " + ServerHuntData.wscTargetSystem + L" alive!");
			}
		}

	}
	
	void Start_PlayerHunt(uint iClientID, const std::wstring& wscParam)
	{
		// The last error.
		HK_ERROR err;

		//Init Cash
		int cash;
		
		// Get the current character name
		std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
		
		//Check HUNT_STATE_DISCONNECTED
		if (ServerHuntData.eState != HUNT_STATE_DISCONNECTED)
		{
			//Get cash from the user command.
			std::wstring wscCash = GetParam(wscParam, L' ', 0);
			wscCash = ReplaceStr(wscCash, L".", L"");
			wscCash = ReplaceStr(wscCash, L",", L"");
			wscCash = ReplaceStr(wscCash, L"$", L"");
			cash = ToInt(wscCash);
			if (cash <= 0) {
				PrintUserCmdText(iClientID, L"ERR Invalid parameter");
				PrintUserCmdText(iClientID, L"/playerhunt <credits>");
				return;
			}

			// Read the current number of credits for the player
			// and check that the character has enough cash.
			int iCash = 0;
			if ((err = HkGetCash(wscCharname, iCash)) != HKE_OK) {
				PrintUserCmdText(iClientID, L"ERR " + HkErrGetText(err));
				return;
			}
			if (cash < set_iMinCredits || cash < 0) {
				PrintUserCmdText(iClientID, L"ERR PlayerHunt amount too small, minimum amount " + ToMoneyStr(set_iMinCredits) + L" credits");
				return;
			}
			if (iCash < cash) {
				PrintUserCmdText(iClientID, L"ERR Insufficient credits");
				return;
			}
		}
		else {
			cash = ServerHuntData.iCredits;
		}


		//Check state with switch case
		switch (ServerHuntData.eState) 
		{
			case HUNT_STATE_NONE:
			{
				//We can start a hunt

				//Get Base of Player
				uint iBaseIDPlayer;
				pub::Player::GetBase(iClientID, iBaseIDPlayer);

				//Check if player is docked
				if (iBaseIDPlayer == 0) {
					//Player is not in a base
					PrintUserCmdText(iClientID, L"ERR You are not docked!");
					return;
				}

				//Generate new TargetBase
				BaseData TargetBase = getTargetBase(iClientID);

				//Get RealName
				HkLoadStringDLLs();

				struct Universe::IBase* baseinfo = Universe::get_base(TargetBase.iBaseID);
				std::wstring wscBasename = HkGetWStringFromIDS(baseinfo->iBaseIDS);

				const struct Universe::ISystem* sysinfo = Universe::get_system(TargetBase.iSystemID);
				std::wstring wscSystemname = HkGetWStringFromIDS(sysinfo->strid_name);


				// Remove cash from current character and save it checking that the
				// save completes
				if ((err = HkAddCash(wscCharname, 0 - cash)) != HKE_OK) {
					PrintUserCmdText(iClientID,
						L"ERR Remove cash failed err=" + HkErrGetText(err));
					return;
				}

				if (HkAntiCheat(iClientID) != HKE_OK) {
					PrintUserCmdText(iClientID, L"ERR Transfer failed");
					return;
				}
				HkSaveChar(iClientID);

				//Save Data
				ServerHuntData.eState = HUNT_STATE_HUNTING;
				ServerHuntData.iCredits = cash;
				ServerHuntData.iTargetBase = TargetBase.iBaseID;
				ServerHuntData.wscTargetBase = wscBasename;
				ServerHuntData.iTargetSystem = TargetBase.iSystemID;
				ServerHuntData.wscTargetSystem = wscSystemname;
				ServerHuntData.tmHuntTime = timeInMS();
				ServerHuntData.wscCharname = wscCharname;

				//Print Announcement
				HkMsgU(wscCharname + L" has started a PlayerHunt! The target System is " + wscSystemname);
				HkMsgU(L"Hunt the player to get the reward of " + std::to_wstring(cash) +  L" Credits yourself!");				
				PrintUserCmdText(iClientID, L"Reach "+wscBasename+L" in System " + wscSystemname + L" alive!");
				
				return;
			}
			case HUNT_STATE_DISCONNECTED:
			{
				//We can capture the hunt
				//Get Base of Player
				uint iBaseIDPlayer;
				pub::Player::GetBase(iClientID, iBaseIDPlayer);

				//Check if player is docked
				if (iBaseIDPlayer == 0) {
					//Player is not in a base
					PrintUserCmdText(iClientID, L"ERR You are not docked!");
					return;
				}

				//Generate new TargetBase
				BaseData TargetBase = getTargetBase(iClientID);

				//Get RealName
				HkLoadStringDLLs();

				struct Universe::IBase* baseinfo = Universe::get_base(TargetBase.iBaseID);
				std::wstring wscBasename = HkGetWStringFromIDS(baseinfo->iBaseIDS);

				const struct Universe::ISystem* sysinfo = Universe::get_system(TargetBase.iSystemID);
				std::wstring wscSystemname = HkGetWStringFromIDS(sysinfo->strid_name);
				ServerHuntData.wscCharname = wscCharname;

				//Save Data
				ServerHuntData.eState = HUNT_STATE_HUNTING;
				ServerHuntData.iCredits = cash;
				ServerHuntData.iTargetBase = TargetBase.iBaseID;
				ServerHuntData.wscTargetBase = wscBasename;
				ServerHuntData.iTargetSystem = TargetBase.iSystemID;
				ServerHuntData.wscTargetSystem = wscSystemname;
				ServerHuntData.tmHuntTime = timeInMS();
				ServerHuntData.wscCharname = wscCharname;

				//Print Announcement
				HkMsgU(wscCharname + L" has captured the Hunt! The new target system is " + wscSystemname);
				HkMsgU(L"Hunt the player to get the reward of " + std::to_wstring(cash) + L" Credits yourself!");
				PrintUserCmdText(iClientID, L"Reach " + wscBasename + L" in System " + wscSystemname + L" alive!");

				return;
			}
			case HUNT_STATE_HUNTING:
			{
				//We must wait for the next hunt
				PrintUserCmdText(iClientID, L"ERR A Hunt is active alredy! Kill " + ServerHuntData.wscCharname + L" to get the reward!");
				return;
			}
		}		
	}
	






}