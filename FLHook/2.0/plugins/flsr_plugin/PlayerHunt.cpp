#include "Main.h"

namespace PlayerHunt {

	float set_fRewardMultiplicator;
	int set_iMinCredits;
	int set_iMaxCredits;
	int set_iMinTargetSystemDistance;
	int set_iMinPlayer;

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
		std::string scPluginCfgFile = std::string(szCurDir) + Globals::PLUGIN_CONFIG_FILE;

		set_fRewardMultiplicator = IniGetF(scPluginCfgFile, "PlayerHunt", "RewardMultiplicator", 0.5f);
		set_iMinTargetSystemDistance = IniGetI(scPluginCfgFile, "PlayerHunt", "MinTargetSystemDistance", 4);
		set_iMinCredits = IniGetI(scPluginCfgFile, "PlayerHunt", "MinCredits", 50000);
		set_iMaxCredits = IniGetI(scPluginCfgFile, "PlayerHunt", "MaxCredits", 100000000);
		set_iMinPlayer = IniGetI(scPluginCfgFile, "PlayerHunt", "MinPlayer", 4);




	}

	uint getRandomSysteminRange(uint iClientID)
	{	
		uint iLoop;

		//Get player system
		uint iSysIDPlayer;
		pub::Player::GetSystem(iClientID, iSysIDPlayer);
		
		//Get player system nickname
		char szSystemnamePlayer[1024] = "";
		pub::GetSystemNickname(szSystemnamePlayer, sizeof(szSystemnamePlayer), iSysIDPlayer);
		
		//Get all systems
		std::vector<std::string> systems;
		struct Universe::ISystem* sysinfo = Universe::GetFirstSystem();
		iLoop = 0;
		while (sysinfo && iLoop < 25) {
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
			
			iLoop++;
			sysinfo = Universe::GetNextSystem();
		}

		//Get random system in min Range
		int range = 0;
		std::string randomSystem;
		iLoop = 0;
		while (range <= set_iMinTargetSystemDistance && iLoop < 25) {
			int random = rand() % systems.size();
			randomSystem = systems[random];
			range = Tools::CountShortestPath(szSystemnamePlayer, randomSystem);
			iLoop++;
		}			

		if (randomSystem == "")
			return 0;
		
		uint iSystemID = Universe::get_system_id(randomSystem.c_str());
		
		return iSystemID;
	}

	BaseData getRandomBaseInSystem(uint iPlayerSystemID, uint iClientID)
	{
		//Get Factions fpr Reputation Check
		std::list<Tools::RepCB> lstTagFactions = Tools::HkGetFactions();

		//Get all bases of System
		std::vector<BaseData> bases;
		//ConPrint(L"loopstart\n");

		if (iPlayerSystemID == 0)
		{
			//ConPrint(L"No bases found\n");
			return BaseData();
		}

		
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
			//ConPrint(L"No bases found\n");
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
		uint iLoop = 0;
		while (TargetBase.scBaseNickname == "" && iLoop < 25)
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
			
			iLoop++;
		}
			
		return TargetBase;
	}

	void CalcReward()
	{
		int iReward = ServerHuntData.iReward + (ServerHuntData.iHuntCredits * set_fRewardMultiplicator);
		ServerHuntData.iReward = iReward;
	}
	
	void PlayerHuntMulti(uint iClientID, uint iPlayerSystemID)
	{
		//Check if Hunt is active
		if (ServerHuntData.eState == HUNT_STATE_HUNTING)
		{
			//Calc only once per System
			//Get Player System
			char szSystemnamePlayer[1024] = "";
			pub::GetSystemNickname(szSystemnamePlayer, sizeof(szSystemnamePlayer), iPlayerSystemID);
			std::string scSystemnamePlayer = szSystemnamePlayer;
			
			//While List
			std::list<std::string>::iterator iterSystems = ServerHuntData.lSystems.begin();
			bool bSystemFound = false;
			while (iterSystems != ServerHuntData.lSystems.end()) {
				if (*iterSystems == scSystemnamePlayer)
				{
					bSystemFound = true;
					break;
				}
				iterSystems++;
			}

			//PayReward
			if (!bSystemFound)
			{
				//Get RealName
				HkLoadStringDLLs();

				const struct Universe::ISystem* sysinfo = Universe::get_system(iPlayerSystemID);
				std::wstring wscClearSystemname = HkGetWStringFromIDS(sysinfo->strid_name);

				CalcReward();
				
				//The new PlayerHunt reward is 20,000 credits!
				HkMsgU(L"The new player hunt reward is " + ToMoneyStr(ServerHuntData.iReward) + L" credits!");
				//Hunt down the player Peter in Sigma 19 and get the reward!
				HkMsgU(L"Hunt down the player " + ServerHuntData.wscCharname + L" in " + wscClearSystemname + L" and get the reward!");
				ServerHuntData.lSystems.push_back(scSystemnamePlayer);
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
					//Peter has reached System Blubber, kill the player before he docks at Planet Blab.
					HkMsgU(ServerHuntData.wscCharname + L" has reached system " + ServerHuntData.wscTargetSystem + L", kill the player before he docks at " + ServerHuntData.wscTargetBase + L".");
					//You have reached the target system. Be careful, players could be near!
					PrintUserCmdText(iClientID, L"You have reached the target system. Be careful, players could be near!");
				}

				PlayerHuntMulti(iClientID, iPlayerSystemID);
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
					HkAddCash(wscCharname, ServerHuntData.iReward);

					//Print Announcement
					//Peter has reached Planet Blab! The player has received a reward of 12 credits!
					HkMsgU(ServerHuntData.wscCharname + L" has reached " + ServerHuntData.wscTargetBase + L"! The player has received a reward of " + ToMoneyStr(ServerHuntData.iReward) + L" credits!");
					//You have survived!
					PrintUserCmdText(iClientID, L"You have survived!");
					ServerHuntData.eState = HUNT_STATE_NONE;
					
					//Reset List
					ServerHuntData.lSystems.clear();
				}
			}
		}
	}

	void CheckDisConnect(uint iClientID)
	{
		


		//Check if Hunt is active
		if (ServerHuntData.eState == HUNT_STATE_HUNTING)
		{
			if (!Tools::isValidPlayer(iClientID, false))
				return;
				

			//Check is Player has a Hunt
			// Get the character name

			std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
	

			if (ServerHuntData.wscCharname == wscCharname)
			{
				//We have the Hunt
				//Peter has disconnected! Dock at a base and take over the mission with /playerhunt
				HkMsgU(ServerHuntData.wscCharname + L" has disconnected! Dock at a base and take over the mission with /playerhunt");
				ServerHuntData.eState = HUNT_STATE_LOST;
				
			}
		}
	}

	void CheckDied(uint iClientID,uint iClientKillerID, Tools::eDeathTypes DeathType)
	{

		//Check for DeathType - PVE Kill
		if (DeathType == Tools::PVE)
		{
			//ConPrint(L"PVEKILL\n");
			//Check if Hunt is active
			if (ServerHuntData.eState == HUNT_STATE_HUNTING && Tools::isValidPlayer(iClientID, true))
			{

				//Check is Player has a Hunt
				// Get the current character name
				std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
				if (ServerHuntData.wscCharname == wscCharname)
				{
					//We have the Hunt
					//Peter has been killed by an NPC! Dock at a base and take over the mission with /playerhunt
					HkMsgU(ServerHuntData.wscCharname + L" has been killed by an NPC! Dock at a base and take over the mission with /playerhunt");
					ServerHuntData.eState = HUNT_STATE_LOST;
					return;
					
				}
			}
		}
		
		//Check for DeathType - Suicide Kill
		if (DeathType == Tools::SUICIDE || DeathType == Tools::KILLEDHIMSELF || DeathType == Tools::HASDIED)
		{

			//Check if Hunt is active
			if (ServerHuntData.eState == HUNT_STATE_HUNTING && Tools::isValidPlayer(iClientID, true))
			{

				//Check is Player has a Hunt
				// Get the current character name
				std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
				if (ServerHuntData.wscCharname == wscCharname)
				{
					//We have the Hunt
					//Peter has committed suicide! Dock at a base and take over the mission with /playerhunt
					HkMsgU(wscCharname + L" has committed suicide! Dock at a base and take over the mission with /playerhunt");
					ServerHuntData.eState = HUNT_STATE_LOST;
					return;

				}
			}
		}

		//Check for DeathType - PVP Kill
		if (DeathType == Tools::PVP)
		{

			if (ServerHuntData.eState == HUNT_STATE_HUNTING && Tools::isValidPlayer(iClientID, true) && Tools::isValidPlayer(iClientKillerID, true))
			{
				//Check is Player has a Hunt
				// Get the current character name
				std::wstring wscCharnameClient = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
				std::wstring wscCharnameKiller = (const wchar_t*)Players.GetActiveCharacterName(iClientKillerID);

				if (ServerHuntData.wscCharname == wscCharnameClient)
				{
					//We have the Hunt

					//Update the Hunt
					ServerHuntData.wscCharname = wscCharnameKiller;

					//Peter died! The hunted player is now Ursula!
					HkMsgU(wscCharnameClient + L" died! The hunted player is now " + wscCharnameKiller + L"!");
					//Hunt the player to get the reward of 235 credits yourself!
					HkMsgU(L"Hunt the player to get the reward of " + ToMoneyStr(ServerHuntData.iReward) + L" credits yourself!");
					//Reach Suppe in system Kapser alive!
					PrintUserCmdText(iClientID, L"Reach " + ServerHuntData.wscTargetBase + L" in system " + ServerHuntData.wscTargetSystem + L" alive!");
					return;
				}
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
	
		//Get Online Players
		uint iPlayersCount = 0;
		struct PlayerData* pPD = 0;
		while (pPD = Players.traverse_active(pPD)) {
			uint ipClientID = HkGetClientIdFromPD(pPD);
			if (HkIsInCharSelectMenu(ipClientID)) {
				continue;
			}
			iPlayersCount++;
		}
		
		if (iPlayersCount < set_iMinPlayer)
		{
			//At least four players must be online to start a hunt! 
			PrintUserCmdText(iClientID, L"At least " + std::to_wstring(set_iMinPlayer) + L" players must be online to start a hunt!");
			return;
		}
		
		
		//Get player system
		uint iSysIDPlayer;
		pub::Player::GetSystem(iClientID, iSysIDPlayer);

		//Get player system nickname
		char szSystemnamePlayer[1024] = "";
		pub::GetSystemNickname(szSystemnamePlayer, sizeof(szSystemnamePlayer), iSysIDPlayer);
		std::string scSystemNicknamePlayer = szSystemnamePlayer;
		
		if (scSystemNicknamePlayer == "start")
		{
			//You can't use this command in the start system.
			PrintUserCmdText(iClientID, L"You can't use this command in the start system.");
			return;
		}
		
		//Check Whitelist
		bool bWhitelisted = false;
		for (std::vector<std::string>::iterator t = SystemWhitelist.begin(); t != SystemWhitelist.end(); ++t)
		{
			
			if (scSystemNicknamePlayer.find(*t) != std::string::npos) {
				//ConPrint(stows(scSystemNicknamePlayer) + L" is whitelisted\n");
				bWhitelisted = true;
				continue;
			}

		}

		if (!bWhitelisted)
		{
			//You can't use this command in this system.
			PrintUserCmdText(iClientID, L"You can't use this command in this system.");
			return;
		}
		
		//ConPrint bWhitelisted
		//ConPrint(L"bWhitelisted: " + stows(std::to_string(bWhitelisted)) + L"\n");
		
		//Check HUNT_STATE_DISCONNECTED
		if (ServerHuntData.eState == HUNT_STATE_NONE)
		{
			//Get cash from the user command.
			std::wstring wscCash = GetParam(wscParam, L' ', 0);
			wscCash = ReplaceStr(wscCash, L".", L"");
			wscCash = ReplaceStr(wscCash, L",", L"");
			wscCash = ReplaceStr(wscCash, L"$", L"");
			cash = ToInt(wscCash);
			if (cash <= 0) {
				//ERR: Invalid parameter: /playerhunt <credits>
				PrintUserCmdText(iClientID, L"ERR: Invalid parameter: /playerhunt <credits>");
				return;
			}

			// Read the current number of credits for the player
			// and check that the character has enough cash.
			int iCash = 0;
			if ((err = HkGetCash(wscCharname, iCash)) != HKE_OK) {
				PrintUserCmdText(iClientID, L"ERR: " + HkErrGetText(err));
				return;
			}
			if (cash < set_iMinCredits || cash < 0) {
				//ERR: The PlayerHunt amount is too small. The minimum amount is [min credits] credits.
				PrintUserCmdText(iClientID, L"ERR: The player hunt amount is too small. The minimum amount is " + ToMoneyStr(set_iMinCredits) + L" credits.");
				return;
			}
			if (cash > set_iMaxCredits) {
				// ERROR: The player hunt amount is too high. The maximum amount is [max credits].
				PrintUserCmdText(iClientID, L"ERR: The player hunt amount is too high. The maximum amount is " + ToMoneyStr(set_iMaxCredits) + L" credits.");
				return;
			}
			if (iCash < cash) {
				//ERR: Insufficient credits.
				PrintUserCmdText(iClientID, L"ERR: Insufficient credits.");
				return;
			}
		}
		else {
			cash = ServerHuntData.iReward;
		}


		//Check state with switch case
		switch (ServerHuntData.eState) 
		{
			case HUNT_STATE_NONE:
			{
				//We can start a hunt

				//Generate new TargetBase
				BaseData TargetBase = getTargetBase(iClientID);

				//Get RealName
				HkLoadStringDLLs();

				const struct Universe::IBase* baseinfo = Universe::get_base(TargetBase.iBaseID);
				std::wstring wscBasename = HkGetWStringFromIDS(baseinfo->iBaseIDS);

				const struct Universe::ISystem* sysinfo = Universe::get_system(TargetBase.iSystemID);
				std::wstring wscSystemname = HkGetWStringFromIDS(sysinfo->strid_name);


				// Remove cash from current character and save it checking that the
				// save completes
				if ((err = HkAddCash(wscCharname, 0 - cash)) != HKE_OK) {
					PrintUserCmdText(iClientID,L"ERR: Remove cash failed err=" + HkErrGetText(err));
					return;
				}

				if (HkAntiCheat(iClientID) != HKE_OK) {
					PrintUserCmdText(iClientID, L"ERR: payment failed");
					return;
				}
				HkSaveChar(iClientID);

				//Save Data
				ServerHuntData.eState = HUNT_STATE_HUNTING;
				ServerHuntData.iHuntCredits = cash;
				ServerHuntData.iReward = cash;
				ServerHuntData.iTargetBase = TargetBase.iBaseID;
				ServerHuntData.wscTargetBase = wscBasename;
				ServerHuntData.iTargetSystem = TargetBase.iSystemID;
				ServerHuntData.wscTargetSystem = wscSystemname;
				ServerHuntData.wscCharname = wscCharname;
				ServerHuntData.lSystems.push_back(scSystemNicknamePlayer);

				//Print Announcement
				//Peter has started a player hunt! The target system is New Tokyo.
				HkMsgU(wscCharname + L" has started a player hunt! The target system is " + wscSystemname + L".");
				//
				//Hunt the player to get the reward of 235 credits yourself!
				HkMsgU(L"Hunt the player to get the reward of " + ToMoneyStr(cash) +  L" credits yourself!");

				//Reach Suppe in system Kapser alive!
				PrintUserCmdText(iClientID, L"Reach " + ServerHuntData.wscTargetBase + L" in system " + ServerHuntData.wscTargetSystem + L" alive!");

				//Discord Chat
				std::wstring wscMessage = wscCharname + L" has started a player hunt! The target system is " + wscSystemname + L"!";

				{//Mutex
					std::lock_guard<std::mutex> lock(m_Mutex);

					//Discord
					Discord::ChatMessage ChatMsg;
					ChatMsg.wscCharname = L"FL:SR";
					ChatMsg.wscChatMessage = wscMessage;

					Discord::lChatMessages.push_back(ChatMsg);


				}				
				return;
			}
			case HUNT_STATE_LOST:
			{
				//We can capture the hunt
				//Get Base of Player
				uint iBaseIDPlayer;
				pub::Player::GetBase(iClientID, iBaseIDPlayer);

				//Check if player is docked
				if (iBaseIDPlayer == 0) {
					//Player is not in a base
					PrintUserCmdText(iClientID, L"You need to be docked to take over the hunt!");
					return;
				}

				//Generate new TargetBase
				BaseData TargetBase = getTargetBase(iClientID);

				//Get RealName
				HkLoadStringDLLs();

				const struct Universe::IBase* baseinfo = Universe::get_base(TargetBase.iBaseID);
				std::wstring wscBasename = HkGetWStringFromIDS(baseinfo->iBaseIDS);

				const struct Universe::ISystem* sysinfo = Universe::get_system(TargetBase.iSystemID);
				std::wstring wscSystemname = HkGetWStringFromIDS(sysinfo->strid_name);
				ServerHuntData.wscCharname = wscCharname;

				//Save Data
				ServerHuntData.eState = HUNT_STATE_HUNTING;
				ServerHuntData.iReward = cash;
				ServerHuntData.iTargetBase = TargetBase.iBaseID;
				ServerHuntData.wscTargetBase = wscBasename;
				ServerHuntData.iTargetSystem = TargetBase.iSystemID;
				ServerHuntData.wscTargetSystem = wscSystemname;
				ServerHuntData.wscCharname = wscCharname;
				ServerHuntData.lSystems.push_back(scSystemNicknamePlayer);

				//Print Announcement
				//Peter has started a player hunt! The target system is New Tokyo.
				HkMsgU(wscCharname + L" has taken over the hunt! The target system is " + wscSystemname + L".");

				//Hunt the player to get the reward of 235 credits yourself!
				HkMsgU(L"Hunt the player to get the reward of " + ToMoneyStr(cash) + L" credits yourself!");

				//Reach Suppe in system Kapser alive!
				PrintUserCmdText(iClientID, L"Reach " + ServerHuntData.wscTargetBase + L" in system " + ServerHuntData.wscTargetSystem + L" alive!");

				return;
			}
			case HUNT_STATE_HUNTING:
			{
				if (wscCharname == ServerHuntData.wscCharname)
				{
					//Reach Suppe in system Kapser alive!
					PrintUserCmdText(iClientID, L"Reach " + ServerHuntData.wscTargetBase + L" in system " + ServerHuntData.wscTargetSystem + L" alive!");
				}
				else {
					//A hunt is active already! Kill Bobbel to get the reward!
					PrintUserCmdText(iClientID, L"A hunt is active already! Kill " + ServerHuntData.wscCharname + L" to get the reward!");
				}
				return;
			}
		}		
	}
	






}