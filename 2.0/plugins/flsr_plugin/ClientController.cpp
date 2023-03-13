#include "main.h"


namespace ClientController {

    std::list<PathSelection::OpenUnlawfulMod> lOpenUnlawfulMods;

    // Client Controller
    void Send_ControlMsg(bool sHook, uint iClientID, std::wstring wscText, ...) {
        wchar_t wszBuf[1024 * 8] = L"";
        va_list marker;
        va_start(marker, wscText);
        _vsnwprintf(wszBuf, sizeof(wszBuf) - 1, wscText.c_str(), marker);
        std::wstring wscControlMsg;
		
        if (sHook)
        {
            wscControlMsg = L" $$% ";
        }
        else {
            wscControlMsg = L" $$$# ";
        }
            wscControlMsg += wszBuf;

        std::wstring wscXML = L"<TEXT>" + XMLText(wscControlMsg) + L"</TEXT>";
        
		//ConPrint(L"SIZE: " + std::to_wstring(wscXML.size()) + L"\n");

        HkFMsg(iClientID, wscXML);
    }

    void Handle_ClientControlMsg(CHAT_ID cId, unsigned long lP1, void const* rdlReader, CHAT_ID cIdTo, int iP2)
    {
		returncode = DEFAULT_RETURNCODE;       
      
        // extract text from rdlReader
        BinaryRDLReader rdl;
        wchar_t wszBuf[1024] = L"";
        uint iRet1;
        rdl.extract_text_from_buffer((unsigned short*)wszBuf, sizeof(wszBuf), iRet1, (const char*)rdlReader, lP1);
        std::wstring wscBuf = wszBuf;
		std::string scBuf = wstos(wscBuf);
        uint iClientID = cId.iID;

        //ConPrint(L"Triggered %s\n", wscBuf);
        
		//Check if it is a ClientController Message
        if (Tools::startsWith(scBuf, " $%$ "))
        {
            //Get Data
			std::string scData = scBuf.substr(5);

            //ConPrint(L"Triggered %s\n", stows(scData));

			
            //EquipWhiteList
            if (Modules::GetModuleState("EquipWhiteListModule"))
            {
                if (Tools::startsWith(scData, "MOUNTING "))
                {
                    //Get Data
                    std::string scGoodID = scData.substr(9);
                    uint iGoodID = ToInt(stows(scGoodID));
                    //EquipWhiteList::CC_CheckEquipWhiteList(iClientID, iGoodID);
                }
				if (Tools::startsWith(scData, "shipID: "))
				{
                    //Get Data
					std::string scShipArchID = scData.substr(8);
                    uint iShipArchID = stoi(scShipArchID);
                    EquipWhiteList::SendList(iShipArchID, iClientID, true);
				}
			}
            //Cloak
            if (Modules::GetModuleState("CloakModule"))
            {
                if (Tools::startsWith(scData, "Power: "))
                {
                    //GetCharfile
                    std::wstring wscCharFileName;
                    HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName);
                    
					//Get Data
					std::string scEnergy = scData.substr(7);
					float fEnergy = stof(scEnergy);
					Cloak::mPlayerCloakData[wscCharFileName].fEnergy = fEnergy;
                }

                if (Tools::startsWith(scData, "PLAYERCLOAK"))
                {
					Commands::UserCmd_CLOAK(iClientID, L"");
                }

                if (Tools::startsWith(scData, "PLAYERUNCLOAK"))
                {
                    Commands::UserCmd_UNCLOAK(iClientID, L"");
                }

                if (Tools::startsWith(scData, "F1PRESSED"))
                {
                    
                    Commands::UserCmd_UNCLOAK(iClientID, L"");
                    //ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
                    Cloak::InstallCloak(iClientID);

                }
            }
            //Carrier Module
			if (Modules::GetModuleState("CarrierModule"))
			{
                if (Tools::startsWith(scData, "ACCEPTALLDOCKREQUESTS"))
                {
                    Docking::DOCKACCEPT_ALL(iClientID);
                }
				if (Tools::startsWith(scData, "USER_SENDCARRIERREQUEST"))
				{
					Commands::UserCMD_DOCKREQUEST(iClientID, L"");
				}
			}
            //Depot Module
            if (Modules::GetModuleState("DepotModule"))
            {
                if (Tools::startsWith(scData, "DEPOTOPEN"))
                {
					Depot::PlayerDepotOpen(iClientID);
                }
                if (Tools::startsWith(scData, "GET_CARGO"))
                {
                    Depot::GetPlayerEquip(iClientID);
                }
            }
            //PathSelection
			if (Modules::GetModuleState("PathSelection"))
			{
                if (Tools::startsWith(scData, "lawful"))
                {
                    std::wstring ID = HkGetAccountIDByClientID(iClientID);
                   // ConPrint(ID + L"\n");
                   // ConPrint(stows(scData) + L"\n");

                    CAccount* acc = Players.FindAccountFromClientID(iClientID);
                    std::wstring wscAccDir;
                    HkGetAccountDirName(acc, wscAccDir);
                   // ConPrint(wscAccDir + L"\n");

                    std::string scCharname = scData.substr(9);
                   // ConPrint(std::to_wstring(CreateID(scCharname.c_str())) + L"\n");

                    PathSelection::SetUnlawful(iClientID, scCharname, "false");
                }
                if (Tools::startsWith(scData, "unlawful"))
                {
                    std::wstring ID = HkGetAccountIDByClientID(iClientID);
                    //ConPrint(ID + L"\n");
                    //ConPrint(stows(scData) + L"\n");

                    CAccount* acc = Players.FindAccountFromClientID(iClientID);
                    std::wstring wscAccDir;
                    HkGetAccountDirName(acc, wscAccDir);
                    //ConPrint(wscAccDir + L"\n");

                    std::string scCharname = scData.substr(9);
                    //ConPrint(std::to_wstring(CreateID(scCharname.c_str())) + L"\n");
                    
                    PathSelection::OpenUnlawfulMod NewOpenUnlawfulMod;
					NewOpenUnlawfulMod.scAccountID = ID;
                    NewOpenUnlawfulMod.wscAccDir = wscAccDir;
					NewOpenUnlawfulMod.scCharname = scCharname;
					NewOpenUnlawfulMod.iClientID = iClientID;
                    
					

                    PathSelection::lOpenUnlawfulMods.push_back(NewOpenUnlawfulMod);
                    
                       
                    PathSelection::SetUnlawful(iClientID, scCharname, "true");
                    
                    //ClientController::Send_ControlMsg(false, iClientID, L"_AltDisconnectText");
                }
			}

        }
    }
	
}

