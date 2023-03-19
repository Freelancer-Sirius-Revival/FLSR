#include "Main.h"

namespace EquipWhiteList {

	std::list<EquipWhiteListEntry> lEquipWhiteList;

	void LoadEquipWhiteList()
	{
		//Read Settings
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		std::string scEquipWhiteList = std::string(szCurDir) + Equip_WHITELIST_FILE;

		//Clear old data
		lEquipWhiteList.clear();

		//Load EquipWhiteList Equipment
		std::list<INISECTIONVALUE> lEquip;
		IniGetSection(scEquipWhiteList, "Equipment", lEquip);

		for (std::list<INISECTIONVALUE>::iterator i = lEquip.begin(); i != lEquip.end(); i++) {
			std::string scEquipNickname = (*i).scValue;
			EquipWhiteListEntry NewEquipWhiteList;
			NewEquipWhiteList.scEquipNickname = scEquipNickname;
			NewEquipWhiteList.iEquipID = CreateID(scEquipNickname.c_str());

			//Load Ships of Equipment
			std::list<INISECTIONVALUE> lShips;
			IniGetSection(scEquipWhiteList, scEquipNickname, lShips);
			std::vector<std::pair<uint, std::string>> vShip; // ShipID, ShipNickname

			if (lShips.size() == 0)
			{
				//ConPrint(stows(scEquipNickname) + L"\n");
				vShip.push_back(std::make_pair(CreateID("none"), "none"));
				NewEquipWhiteList.vShip = vShip;
			}
			else {

				for (std::list<INISECTIONVALUE>::iterator j = lShips.begin(); j != lShips.end(); j++) {
					std::string scShipNickname = (*j).scValue;
					vShip.push_back(std::make_pair(CreateID(scShipNickname.c_str()), scShipNickname));


				}
				NewEquipWhiteList.vShip = vShip;
			}
				
			lEquipWhiteList.push_back(NewEquipWhiteList);
		}
	}

	bool ReqAddItem_CheckEquipWhiteList(unsigned int goodID, char const* hardpoint, int count, float status, bool mounted, ClientId iClientID)
	{
		//Check for WhiteListEquip
		uint iShipArchIDPlayer;
		pub::Player::GetShipID(iClientID, iShipArchIDPlayer);

		std::list<EquipWhiteListEntry>::iterator EquipWhiteListEntry = lEquipWhiteList.begin();
		while (EquipWhiteListEntry != lEquipWhiteList.end()) {


			//Check for Equip
			if (EquipWhiteListEntry->iEquipID == goodID)
			{

				//Loop ShipList
				std::vector<std::pair<uint, std::string>> vShip = EquipWhiteListEntry->vShip;
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
				//NO SHIP
				//Player dont fly a listed Ship
				//PrintUserCmdText(iClientID, L"Player dont fly a listed Ship");

				pub::Player::AddCargo(iClientID, goodID, 1, 1, false);
				pub::Player::SendNNMessage(iClientID, 0x98ACBE43);
				pub::Player::SendNNMessage(iClientID, 0x8D2CDF44);


				return true;
			}
			EquipWhiteListEntry++;
		}
		//Equip not found in WhiteList
		//PrintUserCmdText(iClientID, L"Equip not found in WhiteList");
		return false;
	}

	void SendList(uint iShipArch, ClientId iClientID, bool boldShip)
	{
		//Check for WhiteListEquip
		uint iShipArchIDPlayer = iShipArch;
		std::wstring wscCCMessage = L"";
		std::list<EquipWhiteListEntry>::iterator iterEquipWhiteListEntry = lEquipWhiteList.begin();
		while (iterEquipWhiteListEntry != lEquipWhiteList.end()) {
			std::vector<std::pair<uint, std::string>> vShip = iterEquipWhiteListEntry->vShip;
			int index = 0;
			bool bShipFound = false;
			while (index < vShip.size()) {
				
				std::pair<uint, std::string> pShipData = vShip.at(index);
				if (iShipArchIDPlayer == pShipData.first)
				{
					bShipFound = true;
				}
				if (pShipData.second == "none")
				{
					bShipFound = false;
				}

				index++;
			}
			if (!bShipFound)
			{
				if (wscCCMessage != L"")
				{
					wscCCMessage = wscCCMessage + L"," + std::to_wstring(iterEquipWhiteListEntry->iEquipID);
				}
				else 
				{	
					wscCCMessage = L"Blocklist={" + std::to_wstring(iterEquipWhiteListEntry->iEquipID);
				}
			}
			iterEquipWhiteListEntry++;
		}
		if (boldShip)
		{
			//Check for WhiteListEquip
			pub::Player::GetShipID(iClientID, iShipArchIDPlayer);
			std::list<EquipWhiteListEntry>::iterator iterEquipWhiteListEntry = lEquipWhiteList.begin();
			while (iterEquipWhiteListEntry != lEquipWhiteList.end()) {
				std::vector<std::pair<uint, std::string>> vShip = iterEquipWhiteListEntry->vShip;
				int index = 0;
				bool bShipFound = false;
				while (index < vShip.size()) {

					std::pair<uint, std::string> pShipData = vShip.at(index);
					if (iShipArchIDPlayer == pShipData.first)
					{
						bShipFound = true;
					}
					if (pShipData.second == "none")
					{
						bShipFound = false;
					}

					index++;
				}
				if (!bShipFound)
				{
					if (wscCCMessage != L"")
					{
						wscCCMessage = wscCCMessage + L"," + std::to_wstring(iterEquipWhiteListEntry->iEquipID);
					}
					else
					{
						if (Tools::startsWith(wstos(wscCCMessage), "Blocklist={"))
						{
							wscCCMessage = wscCCMessage + L"," + std::to_wstring(iterEquipWhiteListEntry->iEquipID);
						}
						else
						{
							wscCCMessage = L"Blocklist={" + std::to_wstring(iterEquipWhiteListEntry->iEquipID);
						}
					}
				}
				iterEquipWhiteListEntry++;
			}
		}
		
		wscCCMessage = wscCCMessage + L"}";
		//ConPrint(L"CCMessage: %s\n", wscCCMessage.c_str());
		ClientController::Send_ControlMsg(false, iClientID, wscCCMessage);		
	}
}