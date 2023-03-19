#include "main.h"

namespace PathSelection {

	std::string scSystem;
	std::string scStart_Base;
	uint iCash;
	std::list<PathSelection::Reputation> lReputations;
	std::list<PathSelection::BlockedGate> lBlockedGates;
	std::list<PathSelection::OpenUnlawfulMod> lOpenUnlawfulMods;
	UnlawfulPlayer UnlawfulPlayerData[MAX_CLIENT_ID + 1];



	void LoadPathSelectionSettings()
	{
		//Configpath
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		std::string scPluginCfgFile = std::string(szCurDir) + PATHSELECTION_CONFIG_FILE;

		//Read General 
		PathSelection::scSystem = IniGetS(scPluginCfgFile, "General", "system", "start");
		PathSelection::scStart_Base = IniGetS(scPluginCfgFile, "General", "start_base", "start_02_base");
		PathSelection::iCash = IniGetI(scPluginCfgFile, "General", "credits", 20000);

		//Clear Reputation List
		PathSelection::lReputations.clear();
		std::list<INISECTIONVALUE> lReputationsSettings;
        IniGetSection(scPluginCfgFile, "Reputations", lReputationsSettings);

		//Read Reputations
        for (std::list<INISECTIONVALUE>::iterator i = lReputationsSettings.begin(); i != lReputationsSettings.end(); i++) {
			PathSelection::Reputation NewReputation;
			NewReputation.scFactionName = i->scKey;
			NewReputation.fReputation = std::stof(i->scValue);	
			
			PathSelection::lReputations.push_back(NewReputation);
        }

		//Clear BlockedGate List
		PathSelection::lBlockedGates.clear();
		std::list<INISECTIONVALUE> lBlockedGateSettings;
		IniGetSection(scPluginCfgFile, "BlockedGates", lBlockedGateSettings);

		//Read Blocked-Gates
		for (std::list<INISECTIONVALUE>::iterator i = lBlockedGateSettings.begin(); i != lBlockedGateSettings.end(); i++) {
			PathSelection::BlockedGate NewBlockedGate;
			NewBlockedGate.iGateID = std::stoi(i->scValue);

			PathSelection::lBlockedGates.push_back(NewBlockedGate);
		}

	}

	bool Check_BlockedGate(uint iShip)
	{
		ClientId iClientID = HkGetClientIDByShip(iShip);
		//Check if Player is Unlawful
		if (!UnlawfulPlayerData[iClientID].bisUnlawful)
			return true;

		if (iClientID) {
			// If no target then ignore the request.
			uint iTargetShip;
			pub::SpaceObj::GetTarget(iShip, iTargetShip);
			if (!iTargetShip)
				return true;

			//Check for JumpHole or Gate
			uint iType;
			pub::SpaceObj::GetType(iTargetShip, iType);
			if (iType == OBJ_JUMP_HOLE || iType == OBJ_JUMP_GATE) 
			{
				//Check if Gate is blocked
				for (std::list<BlockedGate>::iterator i = lBlockedGates.begin(); i != lBlockedGates.end(); i++) {
					if (i->iGateID == iTargetShip) {
						//PrintUserCmdText(iClientID, L"Gate is blocked!");
						return false;
					}
				}
			}
			else 
			{
				//If Type is not a Gate or JumpHole ignore the request.
				return true;
			}			
		}
	}

	void Install_Unlawful(ClientId iClientID)
	{
		//Get Paths
		CAccount* acc = Players.FindAccountFromClientID(iClientID);
		std::wstring wscAccDir;
		HkGetAccountDirName(acc, wscAccDir);
		std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;

		//Get Charname
		std::wstring wscCharname = (const wchar_t*)Players.GetActiveCharacterName(iClientID);
		std::string scCharname = wstos(wscCharname);

		//Get "iCharnameID"
		int iCharnameID = CreateID(scCharname.c_str());
		
		//Is Char unlawful
		bool bisCharUnlawful = IniGetB(scUserFile, "FLSR-Unlawful", std::to_string(iCharnameID), false);
		
		//Is Char UnlawfulModed
		bool bisCharUnlawfulMod = IniGetB(scUserFile, "FLSR-Unlawful-Mod", std::to_string(iCharnameID), false);

		
		//Is Char Unlawful
		if (bisCharUnlawful)
		{
			//Set State
			UnlawfulPlayerData[iClientID].bisUnlawful = bisCharUnlawful;
			if (!bisCharUnlawfulMod)
			{
				//Set UnlawfulMod
				IniWrite(scUserFile, "FLSR-Unlawful-Mod", std::to_string(iCharnameID), "true");
				std::list<Reputation>::iterator iterReputations = lReputations.begin();
				while (iterReputations != lReputations.end()) {
					int iPlayerRep;
					pub::Player::GetRep(iClientID, iPlayerRep);
					uint iRepGroupID;
					pub::Reputation::GetReputationGroup(iRepGroupID, iterReputations->scFactionName.c_str());
					pub::Reputation::SetReputation(iPlayerRep, iRepGroupID, iterReputations->fReputation);


					iterReputations++;
				}
				
				
			}

				
		}		
	}

	void SetUnlawful(ClientId iClientID, std::string scCharname, std::string scState)
	{
		//Get Paths
		CAccount* acc = Players.FindAccountFromClientID(iClientID);
		std::wstring wscAccDir;
		HkGetAccountDirName(acc, wscAccDir);

		//Get "iCharnameID"
		int iCharnameID = CreateID(scCharname.c_str());
		
		std::string scUserFile = scAcctPath + wstos(wscAccDir) + FLHOOKUSER_FILE;
		
		//Set Unlawful
		IniWrite(scUserFile, "FLSR-Unlawful", std::to_string(iCharnameID), scState);
		IniWrite(scUserFile, "FLSR-Unlawful-Mod", std::to_string(iCharnameID), "false");


	}

	void ModUnlawfulChar500ms()
	{
		std::list<OpenUnlawfulMod>::iterator iterOpenUnlawfulMods = lOpenUnlawfulMods.begin();
		while (iterOpenUnlawfulMods != lOpenUnlawfulMods.end()) {
							
			//SetCash
			Tools::FLSR_HkFLIniWrite(stows(iterOpenUnlawfulMods->scCharname), L"money", L" " + std::to_wstring(PathSelection::iCash));
			

			//SetSystem
			Tools::FLSR_HkFLIniWrite(stows(iterOpenUnlawfulMods->scCharname), L"system", L" " + stows(PathSelection::scSystem));
				
			//SetBase
			Tools::FLSR_HkFLIniWrite(stows(iterOpenUnlawfulMods->scCharname), L"base", L" " + stows(PathSelection::scStart_Base));
						
			HkKick(ARG_CLIENTID(iterOpenUnlawfulMods->iClientID));

			lOpenUnlawfulMods.erase(iterOpenUnlawfulMods);
			iterOpenUnlawfulMods++;
			
		}
	
	}

}