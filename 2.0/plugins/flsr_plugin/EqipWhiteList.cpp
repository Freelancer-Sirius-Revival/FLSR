#include "Main.h"

namespace EqipWhiteList {

	std::list<EqipWhiteListEntry> lEqipWhiteList;

	void LoadEqipWhiteList()
	{
		//Read Settings
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		std::string scEqipWhiteList = std::string(szCurDir) + EQIP_WHITELIST_FILE;

		//Clear old data
		lEqipWhiteList.clear();

		//Load EqipWhiteList Eqipment
		std::list<INISECTIONVALUE> lEqip;
		IniGetSection(scEqipWhiteList, "Eqipment", lEqip);

		for (std::list<INISECTIONVALUE>::iterator i = lEqip.begin(); i != lEqip.end(); i++) {
			std::string scEqipNickname = (*i).scValue;
			EqipWhiteListEntry NewEqipWhiteList;
			NewEqipWhiteList.scEqipNickname = scEqipNickname;
			NewEqipWhiteList.iEqipID = CreateID(scEqipNickname.c_str());

			//Load Ships of Eqipment
			std::list<INISECTIONVALUE> lShips;
			IniGetSection(scEqipWhiteList, scEqipNickname, lShips);
			std::vector<std::pair<uint, std::string>> vShip; // ShipID, ShipNickname
			for (std::list<INISECTIONVALUE>::iterator j = lShips.begin(); j != lShips.end(); j++) {
				std::string scShipNickname = (*j).scValue;
				vShip.push_back(std::make_pair(CreateID(scShipNickname.c_str()), scShipNickname));


			}
			NewEqipWhiteList.vShip = vShip;

			lEqipWhiteList.push_back(NewEqipWhiteList);
		}
	}

	bool ReqAddItem_CheckEqipWhiteList(unsigned int goodID, char const* hardpoint, int count, float status, bool mounted, uint iClientID)
	{
		//Check for WhiteListEqip
		uint iShipArchIDPlayer;
		pub::Player::GetShipID(iClientID, iShipArchIDPlayer);

		std::list<EqipWhiteListEntry>::iterator EqipWhiteListEntry = lEqipWhiteList.begin();
		while (EqipWhiteListEntry != lEqipWhiteList.end()) {

			//Check for Eqip
			if (EqipWhiteListEntry->iEqipID == goodID)
			{
				//Loop ShipList
				std::vector<std::pair<uint, std::string>> vShip = EqipWhiteListEntry->vShip;
				int index = 0;
				while (index < vShip.size()) {
					std::pair<uint, std::string> pShipData = vShip.at(index);
					if (iShipArchIDPlayer == pShipData.first)
					{
						//Player flys a listed Ship
						//PrintUserCmdText(iClientID, L"Player flys a listed Ship");
						return false;
					}
					else {
						//Player dont fly a listed Ship
						//PrintUserCmdText(iClientID, L"Player dont fly a listed Ship");

						pub::Player::AddCargo(iClientID, goodID, 1, 1, false);
						pub::Player::SendNNMessage(iClientID, 0x98ACBE43);
						pub::Player::SendNNMessage(iClientID, 0x8D2CDF44);


						return true;
					}
				}
			}
			EqipWhiteListEntry++;
		}
		//Eqip not found in WhiteList
		//PrintUserCmdText(iClientID, L"Eqip not found in WhiteList");
		return false;
	}

	void SendList(uint iShipArch, uint iClientID, bool boldShip)
	{
		//Check for WhiteListEqip
		uint iShipArchIDPlayer = iShipArch;
		std::wstring wscCCMessage = L"";
		std::list<EqipWhiteListEntry>::iterator iterEqipWhiteListEntry = lEqipWhiteList.begin();
		while (iterEqipWhiteListEntry != lEqipWhiteList.end()) {
			std::vector<std::pair<uint, std::string>> vShip = iterEqipWhiteListEntry->vShip;
			int index = 0;
			bool bShipFound = false;
			while (index < vShip.size()) {
				
				std::pair<uint, std::string> pShipData = vShip.at(index);
				if (iShipArchIDPlayer == pShipData.first)
				{
					bShipFound = true;
				}

				index++;
			}
			if (!bShipFound)
			{
				if (wscCCMessage != L"")
				{
					wscCCMessage = wscCCMessage + L"," + std::to_wstring(iterEqipWhiteListEntry->iEqipID);
				}
				else 
				{	
					wscCCMessage = L"Blocklist={" + std::to_wstring(iterEqipWhiteListEntry->iEqipID);
				}
			}
			iterEqipWhiteListEntry++;
		}
		if (boldShip)
		{
			//Check for WhiteListEqip
			pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
			std::list<EqipWhiteListEntry>::iterator iterEqipWhiteListEntry = lEqipWhiteList.begin();
			while (iterEqipWhiteListEntry != lEqipWhiteList.end()) {
				std::vector<std::pair<uint, std::string>> vShip = iterEqipWhiteListEntry->vShip;
				int index = 0;
				bool bShipFound = false;
				while (index < vShip.size()) {

					std::pair<uint, std::string> pShipData = vShip.at(index);
					if (iShipArchIDPlayer == pShipData.first)
					{
						bShipFound = true;
					}

					index++;
				}
				if (!bShipFound)
				{
					if (wscCCMessage != L"")
					{
						wscCCMessage = wscCCMessage + L"," + std::to_wstring(iterEqipWhiteListEntry->iEqipID);
					}
					else
					{
						if (Tools::startsWith(wstos(wscCCMessage), "Blocklist={"))
						{
							wscCCMessage = wscCCMessage + L"," + std::to_wstring(iterEqipWhiteListEntry->iEqipID);
						}
						else
						{
							wscCCMessage = L"Blocklist={" + std::to_wstring(iterEqipWhiteListEntry->iEqipID);
						}
					}
				}
				iterEqipWhiteListEntry++;
			}
		}
		
		wscCCMessage = wscCCMessage + L"}";
		ClientController::Send_ControlMsg(false, iClientID, wscCCMessage);		
	}
}