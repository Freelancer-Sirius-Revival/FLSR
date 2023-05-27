#include "Main.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <queue>
#include <unordered_map>


namespace Tools {

    std::list <CMPDump_Exception> lCMPUpdateExceptions;

    float CaclDestroyedHardpointWorth(uint iClientID) {
        std::vector<std::string> DamagedHardPoints =  Tools::GetHardpointsFromCollGroup(iClientID);

        // Player cargo
        int iRemHoldSize;
        std::list<CARGO_INFO> lstCargo;
        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

        // Add mounted Equip to list
        std::list<CARGO_INFO> lstMounted;
        float fValue;
        for (auto &cargo : lstCargo) {
            if (!cargo.bMounted)
                continue;

            // Check Archtype
            Archetype::Equipment *eq = Archetype::GetEquipment(cargo.iArchID);
            auto aType = eq->get_class_type();
            if (aType == Archetype::SHIELD_GENERATOR ||
                aType == Archetype::THRUSTER || aType == Archetype::LAUNCHER ||
                aType == Archetype::GUN || aType == Archetype::MINE_DROPPER ||
                aType == Archetype::COUNTER_MEASURE_DROPPER || aType == Archetype::CLOAKING_DEVICE) {

                for (std::vector<std::string>::iterator it = DamagedHardPoints.begin(); it != DamagedHardPoints.end(); ++it) {
                    if (startsWith(*it, cargo.hardpoint.value)) {
                    
                        const GoodInfo *gi = GoodList_get()->find_by_id(cargo.iArchID);
                        if (gi) {
                            gi = GoodList::find_by_id(gi->iArchID);
                            if (gi) {
                                // float *fResaleFactor = (float *)((char
                                // *)hModServer + 0x8AE78);
                                float fItemValue = gi->fPrice; //* (*fResaleFactor);
                                fValue += fItemValue;
                            }
                        }
                    }
                }
            }
        }
        return fValue;
    }

    std::vector<std::string> GetHardpointsFromCollGroup(uint iClientID) {

            
        std::vector<std::string> vDamagedHardPoints;

        IObjInspectImpl *obj = HkGetInspect(iClientID);
        if (obj) {
            CShip *cship = (CShip *)HkGetEqObjFromObjRW((IObjRW *)obj);
            if (cship) {
                //PrintUserCmdText(iClientID, stows(cship->shiparch()->szName)); //CMP Name
                //PrintUserCmdText(iClientID, stows(cship->shiparch()->cg->name.value)); //CollGroupName

                std::string sShipfile = cship->shiparch()->szName;
                std::filesystem::path f(sShipfile);

                //FLPath
                char szCurDir[MAX_PATH];
                GetCurrentDirectory(sizeof(szCurDir), szCurDir);

                std::string cmpDump = std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string();

                cmpDump = cmpDump + ".cmp_dump";

                if (!std::filesystem::exists(cmpDump)) {
                    //CMPDump not found
					ConPrint(L"ERROR: CMPDump not found for %s \n", stows(cship->shiparch()->szName).c_str());
                    ConPrint(L"ERROR: Dumpfilepath %s \n", stows(cmpDump).c_str());
                    return vDamagedHardPoints;
                }

                std::string CollisionGroupname;
                std::ifstream in(cmpDump);
                std::string str;
                bool damaged = false;
                int iCountLines = 1;
                int iCollGroupDeep = 0;
                int ilastCollGroupDeep = 0;
                std::string lastCollGroupData;
                //List of CMPDumpEntrys
                std::list<CMPDump_Entry> lCMPDump;       
                
                //Parent Map
                std::map<int, ParentMap> mParent;


                //Get CollisionGroups and Hardpoints of CMP
                // Read the next line from File until it reaches the end.
                while (std::getline(in, str)) {
                    // Line contains string of length > 0
                    if (str.size() > 0) {
                        //Create new Entry
						CMPDump_Entry entry;
                        
                        //Count CollisionGroup Deep
						size_t n = std::count(str.begin(), str.end(), '\t');
                        iCollGroupDeep = (int)n;
						
                        //Get last CollGroupDeep on Deepchange
						if (iCollGroupDeep != ilastCollGroupDeep) {                        
                            //Update Parent Map

                            //On Deep ++
                            if (iCollGroupDeep > ilastCollGroupDeep)
                            {
                                if (mParent[iCollGroupDeep].scFirstParent == "")
                                    mParent[iCollGroupDeep].scFirstParent = lastCollGroupData;

                                mParent[iCollGroupDeep].scParent = lastCollGroupData;
                            }

                            //On Deep --
                            if (iCollGroupDeep < ilastCollGroupDeep)
                            {
                                mParent[iCollGroupDeep + 1].scParent = "";
                            }


                            ilastCollGroupDeep = iCollGroupDeep;

						}
                        
						//Check if found Entry is a CollisionGroup or a Hardpoint
                        if (str.find("> ") != std::string::npos) {
                            entry.bisCollGroup = true;

						}
                        else {
                            entry.bisCollGroup = false;
                        }

                        //Check if CollGroup or Hardpoint has a Parent
                        if (iCollGroupDeep > 0) {
                            entry.bhasParent = true;
                            
                            if (entry.bisCollGroup)
                            {
                                entry.scParent = mParent[iCollGroupDeep].scFirstParent;
                            }
                            else {
                                entry.scParent = mParent[iCollGroupDeep].scParent;

                            }

                        }
                        else
                        {
                            entry.scParent = mParent[iCollGroupDeep].scParent;
                            entry.bhasParent = false;
                        }
                        
						//Get Data of CollisionGroup or Hardpoint
                        //CollGrp
                        if (entry.bisCollGroup)
						{
                            entry.scData = str.substr(str.find("> ") + 2);
                            lastCollGroupData = entry.scData;
                        }
                        else 
                        {
                            entry.scData = StringBetween(str, "[", ":");
                        }                                                              
                        
                        lCMPDump.push_back(entry);

                    }
                    iCountLines++;
                }
                in.close();

                //Print CMP Dump - DEBUG
                /*
                for (std::list<CMPDump_Entry>::iterator it = lCMPDump.begin(); it != lCMPDump.end(); ++it) {
                    ConPrint(L"CMPEntry: " + stows(it->scData) + L" isCollGroup: " + std::to_wstring(it->bisCollGroup) + L" hasParent: " + std::to_wstring(it->bhasParent) + L" ParentData: " +stows(it->scParent) + L" \n");

                }
                */

                //Check which Hardpoints are damaged
                std::vector<std::string> vDamagedCollGrps = HkGetCollisionGroups(iClientID, true);
                for (std::vector<std::string>::iterator it = vDamagedCollGrps.begin(); it != vDamagedCollGrps.end(); ++it) {
                   
                    //Get Data
                    std::string scCollGroup_Charfile = *it;
                    int pos = scCollGroup_Charfile.find("=");
                    scCollGroup_Charfile = scCollGroup_Charfile.substr(pos + 1, scCollGroup_Charfile.length());
					std::string scCollGroup = StringBetween(scCollGroup_Charfile, " ", ",");

                    //Print to Console
                    //ConPrint(L"Damaged CollGrp: " + stows(scCollGroup) + L" \n");
                    
                    //Check for Hardpoints on CollGrp
                    for (std::list<CMPDump_Entry>::iterator CMPDump_iter = lCMPDump.begin(); CMPDump_iter != lCMPDump.end(); ++CMPDump_iter) {
						if (CMPDump_iter->scParent == scCollGroup)
						{
							//Print to Console
                            /*
                            if (!CMPDump_iter->bisCollGroup)
                            {
                                ConPrint(L"Hardpoint: " + stows(CMPDump_iter->scData) + L" \n");
                            }
                            else {
                                ConPrint(L"SubCollGrp: " + stows(CMPDump_iter->scData) + L" \n");

                            }
                            */

							//Add Hardpoint to List
                            if (!CMPDump_iter->bisCollGroup)
                            {
                                vDamagedHardPoints.push_back(CMPDump_iter->scData);
                            }
                            else
                            {
								//Found SubCollGroup
                                std::vector<std::string> vSubHardpoints = getHardpoints(CMPDump_iter->scData, lCMPDump);
                                
                                //Combine Vectors
                                vDamagedHardPoints.insert(vDamagedHardPoints.end(), vSubHardpoints.begin(), vSubHardpoints.end());
                                
                            }
							
						}
                        
                    }
                }
                
            }
        }

		//Damaged Hardpoints to ConPrint
		for (std::vector<std::string>::iterator it = vDamagedHardPoints.begin(); it != vDamagedHardPoints.end(); ++it) {
			//ConPrint(L"Damaged Hardpoint: " + stows(*it) + L" \n");
		}
		

        return vDamagedHardPoints;
            
    }

    std::vector<std::string> getHardpoints(std::string scParent, std::list<CMPDump_Entry> CMPList)
    {
		std::vector<std::string> vHardpoints;
        //Damaged CollGroup found
        for (std::list<CMPDump_Entry>::iterator CMPDump_iter = CMPList.begin(); CMPDump_iter != CMPList.end(); ++CMPDump_iter) {
			if (CMPDump_iter->bhasParent)
			{
				if (CMPDump_iter->scParent == scParent)
				{				
					//Add Hardpoint to List
					if (!CMPDump_iter->bisCollGroup)
					{
                       //ConPrint(L"Hardpoint: " + stows(CMPDump_iter->scData) + L" \n");
						vHardpoints.push_back(CMPDump_iter->scData);
                    }
                    else {
                        //ConPrint(L"SubCollGrp: " + stows(CMPDump_iter->scData) + L" \n");
						std::vector<std::string> vSubHardpoints = getHardpoints(CMPDump_iter->scData, CMPList);
						//Combine vectors
						vHardpoints.insert(vHardpoints.end(), vSubHardpoints.begin(), vSubHardpoints.end());
                    }
				}
			}

        }
        
		return vHardpoints;
    }

    bool startsWith(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() &&
           0 == str.compare(0, prefix.size(), prefix);
    }

    bool endsWith(std::string_view str, std::string_view suffix) {
        return str.size() >= suffix.size() &&
               0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }

    // This will return a vector of CollisionGroups
    std::vector<std::string> HkGetCollisionGroups(uint iClientID, bool bOnly) {
        std::vector<std::string> vecString;
        std::vector<int> vecLine;
        std::wstring wscRet = L"";
        std::wstring wscCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);

        wscRet = L"";
        std::wstring wscDir;
        if (!HKHKSUCCESS(HkGetAccountDirName(wscCharname, wscDir)))
            return vecString;

        std::wstring wscFile;
        HkGetCharFileName(wscCharname, wscFile);

        std::string scCharFile = scAcctPath + wstos(wscDir) + "\\" + wstos(wscFile) + ".fl";

        std::ifstream in(scCharFile);
        std::string str;
        int iCountLines = 1;
        // Read the next line from File untill it reaches the end.
        while (std::getline(in, str)) {
            // Line contains string of length > 0 then save it in vector
            if (str.size() > 0) {
                if (startsWith(str, "base_collision_group")) {
                    if (bOnly) {
                        if (!endsWith(str, "1")) {
                            vecString.push_back(str);
                            vecLine.push_back(iCountLines);
                        }
                    } else {
                        vecString.push_back(str);
                        vecLine.push_back(iCountLines);
                    }
                }
            }
            iCountLines++;
        }
        in.close();
        return vecString;
    }

   //Cmpfiles
    void get_cmpExceptions() {
        //Configpath
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + PLUGIN_CONFIG_FILE;

        //Clear List
        lCMPUpdateExceptions.clear();
        std::list<INISECTIONVALUE> lIniSection;
        IniGetSection(scPluginCfgFile, "CMPUpdate-Exceptions", lIniSection);

        //Read Exceptions
        for (std::list<INISECTIONVALUE>::iterator i = lIniSection.begin(); i != lIniSection.end(); i++) {
            CMPDump_Exception NewException;
            NewException.scData = i->scKey;
            lCMPUpdateExceptions.push_back(NewException);
        }
    }


    void get_cmpfiles(const std::filesystem::path& path)
    {
        //FLPath
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);


        //Check if Librelancer-SDK is installed
        if (!std::filesystem::exists(std::string(szCurDir) + std::string(LIBRELANCER_SDK) + "lleditscript.exe"))
        {
            ConPrint(L"Error: Librelancer-SDK is not properly installed please reinstall it to path: \n" + stows(std::string(szCurDir) + std::string(LIBRELANCER_SDK)) + L"\n");
            return;
        }
   
        //Check if Dumpfolder Exists
        if (!std::filesystem::exists(std::string(szCurDir) + CMP_DUMP_FOLDER))
        {
            std::filesystem::create_directory(std::string(szCurDir) + CMP_DUMP_FOLDER);
        }

        //Copy into Dump Folder if not exist or file changed
        for (const auto& p : std::filesystem::recursive_directory_iterator(path)) {
            if (!std::filesystem::is_directory(p)) {
                std::filesystem::path f(p.path());      

                if (endsWith(f.filename().string(), ".cmp") || endsWith(f.filename().string(), ".3db"))
                {
                    //Check if found cmp is a exception
                    bool isException = false;
                    std::list<CMPDump_Exception>::iterator CMPUpdateException = lCMPUpdateExceptions.begin();
                    while (CMPUpdateException != lCMPUpdateExceptions.end()) {
                        if (ToLower(CMPUpdateException->scData) == ToLower(f.parent_path().filename().string() + "\\" + f.filename().string()))
                        {
                           //ConPrint(L"Exception found\n");
                            isException = true;
                            break;
                        }

                        CMPUpdateException++;
                    }

                    //Skip exception
                    if (isException)
                        continue;

                    //Hash Files
                    std::string pathFileA = p.path().string();
                    std::string pathFileB = std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string();

                    if (!std::filesystem::exists(pathFileB))
                    {
                        ConPrint(L"New CMP found: " + stows(f.filename().string()) + L" - ");
                        std::filesystem::copy_file(p.path(), std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string(), std::filesystem::copy_options::update_existing);
                        std::string parameters = "/C \"\"" + std::string(szCurDir) + std::string(LIBRELANCER_SDK) + "lleditscript.exe\"" + " \"" + std::string(szCurDir) + std::string(LIBRELANCER_SDK) + "dumpcmp.cs-script\" \"" + std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string() + "\" > \"" + std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string() + ".cmp_dump\"";
                        ConPrint(L"Dumping - ");
                        system(("cmd " + parameters).c_str());
                        ConPrint(L"done\n");

                        continue;
                    }

                    //Hash File A
                    FILE* fFiletoHashA = fopen(pathFileA.c_str(), "r");
                    MD5Hash md5hasherA;
                    int iDataA;

                    while (!feof(fFiletoHashA)) {
                        iDataA = fgetc(fFiletoHashA);
                        md5hasherA.AddData(&iDataA, 1);
                    }

                    fclose(fFiletoHashA);
                    md5hasherA.CalcValue();

                    //Hash File B
                    FILE* fFiletoHashB = fopen(pathFileB.c_str(), "r");
                    MD5Hash md5hasherB;
                    int iDataB;

                    while (!feof(fFiletoHashB)) {
                        iDataB = fgetc(fFiletoHashB);
                        md5hasherB.AddData(&iDataB, 1);
                    }

                    fclose(fFiletoHashB);
                    md5hasherB.CalcValue();

                    std::string HashFileA = md5hasherA.AsString();
                    std::string HashFileB = md5hasherB.AsString();

                    //Check for different Hashfiles
                    if (HashFileA != HashFileB)
                    {
                        ConPrint(L"Changed CMP found: " + stows(f.filename().string()) + L" - ");
                        //ConPrint(L"HashA: " + stows(HashFileA + "\n"));
                        //ConPrint(L"HashB: " + stows(HashFileB + "\n"));
                        //ConPrint(L"FileChangedA: " + stows(pathFileA + "\n"));
                        //ConPrint(L"FileChangedB: " + stows(pathFileB + "\n"));

                        std::filesystem::copy_file(p.path(), std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string(), std::filesystem::copy_options::update_existing);
                        std::string parameters = "/C \"\"" + std::string(szCurDir) + std::string(LIBRELANCER_SDK) + "lleditscript.exe\"" + " \"" + std::string(szCurDir) + std::string(LIBRELANCER_SDK) + "dumpcmp.cs-script\" \"" + std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string() + "\" > \"" + std::string(szCurDir) + CMP_DUMP_FOLDER + f.filename().string() + ".cmp_dump\"";
                        ConPrint(L"Dumping - ");
                        system(("cmd " + parameters).c_str());
                        ConPrint(L"done\n");

                    }
                }
            }
        }
     
    }
    

    std::wstring CS_wscCharBefore;
    void HkNewPlayerMessage(uint iClientID, struct CHARACTER_ID const &cId) {

        //Valid ID
        HK_ERROR err;
        if (iClientID < 1 || iClientID > MAX_CLIENT_ID)
            return;

        if (!HkIsValidClientID(iClientID)) {
            return;
        }

        const wchar_t *wszCharname = (wchar_t *)Players.GetActiveCharacterName(iClientID);
        CS_wscCharBefore =
            wszCharname ? (wchar_t *)Players.GetActiveCharacterName(iClientID)
                        : L"";

        try {

            // New Player
            std::wstring wscCharFilenameBefore;
            HkGetCharFileName(CS_wscCharBefore, wscCharFilenameBefore);

            wscCharFilenameBefore += L".fl";
            std::wstring wscCharFilename = stows((std::string)cId.szCharFilename);
            char *pAddress = ((char *)hModRemoteClient + ADDR_CLIENT_NEWPLAYER);
            if (!wscCharFilenameBefore.compare(wscCharFilename)) {
                char szNOP[] = {'\x83', '\xC4', '\x08'}; // add esp 08
                WriteProcMem(pAddress, szNOP, 3);
            } else {
                char szORIG[] = {'\xFF', '\x50', '\x24'};
                WriteProcMem(pAddress, szORIG, 3);
            }

        } catch (...) {
            HkAddKickLog(iClientID, L"Corrupt charfile?");
            HkKick(ARG_CLIENTID(iClientID));
            return;
        }
    }

    void HkClearMissionBug(int clientID) {
        struct CHAT_ID To, From;
        To.iID = 0x10004;
        From.iID = clientID;
        char message[8] = "";
        *((int *)(&message[4])) = clientID;
        *((int *)(&message[0])) = 0; // invite
        Server.SubmitChat(From, 8, message, To, -1);
        *((int *)(&message[0])) = 1; // leave
        Server.SubmitChat(From, 8, message, To, -1);
    }

    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789+/";

    static bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

    std::string base64_encode(unsigned char const *bytes_to_encode,
                              unsigned int in_len) {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                                  ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                                  ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; (i < 4); i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                              ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                              ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while ((i++ < 3))
                ret += '=';
        }

        return ret;
    }

    std::string base64_decode(std::string const &encoded_string) {
        int in_len = encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;

        while (in_len-- && (encoded_string[in_] != '=') &&
               is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_];
            in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);

                char_array_3[0] =
                    (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) +
                                  ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] =
                    ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    ret += char_array_3[i];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);

            char_array_3[0] =
                (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) +
                              ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++)
                ret += char_array_3[j];
        }

        return ret;
    }

    void replace_first(std::string &s, std::string const &toReplace,std::string const &replaceWith) {
        std::size_t pos = s.find(toReplace);
        if (pos == std::string::npos)
            return;
        s.replace(pos, toReplace.length(), replaceWith);
    }

    std::string StringBetween(std::string str, std::string first, std::string last)
    {
        int start;
        int end;

        start = str.find(first);
        end = str.find(last, start);
        std::string sreturn;

        if (start != std::string::npos && end != std::string::npos)
        {
            return sreturn = str.substr(start + first.length(), end - start - first.length());
        }
        else {
            return "";
        }
    }

    bool GetB(std::string svalue) {
        if (svalue == "yes" || svalue == "true")
            return true;
        return false;
    }


	//HashMap Stuff
    std::map<uint, HashMap> mNicknameHashMap;

    /// Read an ini file for nicknames and save the associated hashcode
    static void ReadIniNicknameFile(const std::string& filePath) {

        INI_Reader ini;
        if (ini.open(filePath.c_str(), false)) {
            while (ini.read_header()) {
                bool HashComplete = false;
				std::string scNickname = "";
				uint iResID = 0;
                while (ini.read_value()) {
                    if (ini.is_value("nickname")) {
						scNickname = ini.get_value_string();
                    }
                    if (ini.is_value("ids_name")) {
                        iResID = ini.get_value_int(0);
                    }
                    if (iResID != 0 && scNickname != "") {
						HashMap NewHash;
						NewHash.iResID = iResID;
						NewHash.scNickname = scNickname;
                        uint hash = CreateID(Trim(ToLower(scNickname)).c_str());
                        mNicknameHashMap[hash] = NewHash;
                        //ConPrint(stows(scNickname) + L" " + std::to_wstring(iResID) + L" " + std::to_wstring(hash) + L"\n");
                        iResID = 0;
						scNickname = "";
                    }
                }
            }
            ini.close();
        }
    }
    
    /** Read freelancer data to determine mod settings. */
    bool ReadIniNicknames() {

        std::string dataDirPath = DATADIR;
        bool bReturn = false;

        INI_Reader ini;
        mNicknameHashMap.clear();
        if (ini.open("freelancer.ini", false)) {
            while (ini.read_header()) 
            {
                if (ini.is_header("Data")) {
                    while (ini.read_value()) {
                        if (ini.is_value("Equipment")) {
                            ReadIniNicknameFile(dataDirPath + std::string("\\") + ini.get_value_string());
                            bReturn = true;
							
                        }
                        else if (ini.is_value("ships")) {
                            ReadIniNicknameFile(dataDirPath + std::string("\\") + ini.get_value_string());
                            bReturn = true;
                        }
                        else if (ini.is_value("goods")) {
                            ReadIniNicknameFile(dataDirPath + std::string("\\") + ini.get_value_string());
                            bReturn = true;
                        }
                        else if (ini.is_value("loadouts")) {
                            ReadIniNicknameFile(dataDirPath + std::string("\\") + ini.get_value_string());
                            bReturn = true;
                        }
                    }
                }
            }
            ini.close();
		}

        return bReturn;
    }
    

    Matrix Rz(float angleDeg) {
        float angleRad = angleDeg * M_PI / 180.0;
        float c = cos(angleRad);
        float s = sin(angleRad);
        Matrix R;
        R.data[0][0] = c; R.data[0][1] = -s; R.data[0][2] = 0;
        R.data[1][0] = s; R.data[1][1] = c; R.data[1][2] = 0;
        R.data[2][0] = 0; R.data[2][1] = 0; R.data[2][2] = 1;
        return R;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    HK_ERROR FLSR_HkFLIniGet(const std::wstring& wscCharname, const std::wstring& wscKey, std::wstring& wscRet) {
        wscRet = L"";
        std::wstring wscDir;
        if (!HKHKSUCCESS(HkGetAccountDirName(wscCharname, wscDir)))
            return HKE_CHAR_DOES_NOT_EXIST;

        std::wstring wscFile;
        HkGetCharFileName(wscCharname, wscFile);

        std::string scCharFile =
            scAcctPath + wstos(wscDir) + "\\" + wstos(wscFile) + ".fl";
        if (HkIsEncoded(scCharFile)) {
            std::string scCharFileNew = scCharFile + ".ini";
            if (!flc_decode(scCharFile.c_str(), scCharFileNew.c_str()))
                return HKE_COULD_NOT_DECODE_CHARFILE;

            wscRet =
                stows(IniGetS(scCharFileNew, "Player", wstos(wscKey).c_str(), ""));
            DeleteFile(scCharFileNew.c_str());
        }
        else {
            wscRet =
                stows(IniGetS(scCharFile, "Player", wstos(wscKey).c_str(), ""));
        }

        return HKE_OK;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    HK_ERROR FLSR_HkFLIniWrite(const std::wstring& wscCharname,
        const std::wstring& wscKey,
        const std::wstring& wscValue) {
        std::wstring wscDir;
        if (!HKHKSUCCESS(HkGetAccountDirName(wscCharname, wscDir)))
            return HKE_CHAR_DOES_NOT_EXIST;

        std::wstring wscFile;
        HkGetCharFileName(wscCharname, wscFile);

        std::string scCharFile =
            scAcctPath + wstos(wscDir) + "\\" + wstos(wscFile) + ".fl";
        if (HkIsEncoded(scCharFile)) {
            std::string scCharFileNew = scCharFile + ".ini";
            if (!flc_decode(scCharFile.c_str(), scCharFileNew.c_str()))
                return HKE_COULD_NOT_DECODE_CHARFILE;

            IniWrite(scCharFileNew, "Player", wstos(wscKey).c_str(),
                wstos(wscValue).c_str());

            // keep decoded
            DeleteFile(scCharFile.c_str());
            MoveFile(scCharFileNew.c_str(), scCharFile.c_str());
            /*		if(!flc_encode(scCharFileNew.c_str(), scCharFile.c_str()))
                                    return HKE_COULD_NOT_ENCODE_CHARFILE;

                            DeleteFile(scCharFileNew.c_str()); */
        }
        else {
            IniWrite(scCharFile, "Player", wstos(wscKey).c_str(),
                wstos(wscValue).c_str());
        }

        return HKE_OK;
    }

    bool IsPlayerInRange(uint iClientID, uint iClientID2, float fDistance) 
    {
        uint iShip;
        pub::Player::GetShip(iClientID, iShip);

        Vector pos;
        Matrix rot;
        pub::SpaceObj::GetLocation(iShip, pos, rot);

        uint iSystem;
        pub::Player::GetSystem(iClientID, iSystem);

        // Get the this player's current system and location in the system.
        uint iSystem2 = 0;
        pub::Player::GetSystem(iClientID2, iSystem2);
        if (iSystem != iSystem2)
            return false;

        uint iShip2;
        pub::Player::GetShip(iClientID2, iShip2);

        Vector pos2;
        Matrix rot2;
        pub::SpaceObj::GetLocation(iShip2, pos2, rot2);

        // Is player within the specified range of the sending char.
        if (HkDistance3D(pos, pos2) < fDistance)
        {
            return true;
            
        }else
		{
			return false;
		}        
    }

	//Thanks to the original author of this function, I just modified it to fit my needs.
    std::list<RepCB> lstTagFactions;
    std::list<RepCB>* lstSaveFactions;
    _RepCallback saveCallback;

    bool __stdcall RepCallback(RepCB* rep)
    {
        __asm push ecx
        lstSaveFactions->push_back(*rep);
        __asm pop ecx
        return true;
    }

    std::list<RepCB> HkGetFactions()
    {
        std::list<RepCB> lstFactions;
        lstSaveFactions = &lstFactions;
        void* callback = (void*)RepCallback;
        void** obj = &callback;
        Reputation::enumerate((Reputation::RepGroupCB*)&obj);
        
        return lstFactions;
    }

    bool __stdcall RepEnumCallback(RepCB* rep)
    {
        __asm push ecx
        bool bRet = saveCallback(rep);
        __asm pop ecx
        return bRet;
    }

    void HkEnumFactions(_RepCallback callback)
    {
        saveCallback = callback;
        void* enumCallback = (void*)RepEnumCallback;
        void** obj = &enumCallback;
        Reputation::enumerate((Reputation::RepGroupCB*)&obj);
    }



    uint GetiGroupOfFaction(std::wstring wscParam)
    {
        lstTagFactions = HkGetFactions();
		//ConPrint(L"Factions: " + std::to_wstring(lstTagFactions.size()) + L"\n");
        uint iGroup = 0;

        //While List
        std::list<RepCB>::iterator iterFactions = lstTagFactions.begin();
        HkLoadStringDLLs();
        while (iterFactions != lstTagFactions.end()) {
            
            uint iIDS = Reputation::get_short_name(iterFactions->iGroup);
            std::wstring wscFaction = HkGetWStringFromIDS(iIDS);
            wscFaction = ToLower(wscFaction);
            //ConPrint(wscFaction + L"\n");
            if (wscFaction == wscParam)
            {
                iGroup = iterFactions->iGroup;
                break;
            }
            else
            {
                iIDS = Reputation::get_name(iterFactions->iGroup);
                wscFaction = HkGetWStringFromIDS(iIDS);
                wscFaction = ToLower(wscFaction);
                if (wscFaction == wscParam)
                {
                    iGroup = iterFactions->iGroup;
                    break;
                }
            }

            iterFactions++;
        }
        
		return iGroup;
    }

//ShortestPath Stuff start @@@@@@@@@@@@@@@

    // Calculates the shortest path between two systems in a graph
    std::vector<std::string> FindShortestPath(const std::vector<Edge>& edges, const std::string& start, const std::string& end) {
        // Create a map of all nodes and their edges
        std::unordered_map<std::string, std::vector<Edge>> graph;
        for (const auto& edge : edges) {
            graph[edge.start].push_back(edge);
            graph[edge.end].push_back({ edge.end, edge.start, edge.distance });
        }

        // Create a priority queue for the nodes to visit, with the starting node as the first element
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> queue;
        queue.push({ start, 0, "" });

        // Create a map to keep track of the shortest distances found so far
        std::unordered_map<std::string, int> distances;
        distances[start] = 0;

        // Create a map to keep track of the previous node on the shortest path found so far
        std::unordered_map<std::string, std::string> previous;

        // Visit each node in the queue until the end node is found or the queue is empty
        while (!queue.empty()) {
            Node current = queue.top();
            queue.pop();

            if (current.system == end) {
                // The end node has been found, so backtrack to construct the shortest path
                std::vector<std::string> path;
                while (current.system != start) {
                    path.push_back(current.system);
                    current.system = previous[current.system];
                }
                path.push_back(start);
                std::reverse(path.begin(), path.end());
                return path;
            }

            // Visit each neighboring node
            for (const auto& edge : graph[current.system]) {
                std::string neighbor = edge.end;
                int distance = current.distance + edge.distance;
                if (!distances.count(neighbor) || distance < distances[neighbor]) {
                    distances[neighbor] = distance;
                    previous[neighbor] = current.system;
                    queue.push({ neighbor, distance, current.system });
                }
            }
        }

        // The end node was not found, so return an empty path
        return {};
    }

    void ParsePathsFromFile(std::vector<Edge>& edges) {
        std::string filename = std::string(DATADIR) + "\\UNIVERSE\\systems_shortest_path.ini";
        std::ifstream file(filename);
        if (!file.is_open()) {
            ConPrint(L"ParsePathsFromFile-Error: Could not open file " + stows(filename));
            return;
        }



        std::string line;

        while (std::getline(file, line)) {

            //Check Whitelist
            for (std::vector<std::string>::iterator t = PlayerHunt::SystemWhitelist.begin(); t != PlayerHunt::SystemWhitelist.end(); ++t)
            {
                if (~line.find(*t) != std::string::npos) {
                    continue;
                }
            }

            //cout line
            if (line.substr(0, 7) == "Path = ") {

                // Extract the start and end points
                std::string start = line.substr(7, line.find(',', 7) - 7);
                std::string end = line.substr(line.find_last_of(',') + 2);

                // Extract the systems along this path
                std::string systems_str = line.substr(line.find_first_of(',') + 2, line.find_last_of(',') - line.find_first_of(',') - 2);
            
                size_t pos = 0;
                std::string token;
                std::vector<std::string> systems;
                while ((pos = systems_str.find(',')) != std::string::npos) {
                    token = systems_str.substr(0, pos);
                    systems.push_back(token);
                    systems_str.erase(0, pos + 2);
                }
                systems.push_back(systems_str);

                // Remove start and end points from system list
                if (systems.size() > 2) {
                    systems.erase(systems.begin());
                    systems.erase(systems.begin());
                }

                // Create a new edge for each pair of systems
                for (size_t i = 0; i < systems.size() - 1; ++i) {
                    Edge e = { systems[i], systems[i + 1], 1 };
                    edges.push_back(e);
                }
            }
        }

        file.close();
    }

    std::vector<std::string> GetShortestPath(std::string start, std::string end)
    {
        std::vector<std::string> shortest_path;
        std::vector<Edge> edges;
        ParsePathsFromFile(edges);

        if (edges.size() == 0)
            return shortest_path;

        shortest_path = FindShortestPath(edges, start, end);

        return shortest_path;

    }

    int CountShortestPath(std::string start, std::string end)
    {
        std::vector<std::string> shortest_path = GetShortestPath(start,end);
        int numSystems = shortest_path.size();
        return numSystems;

    }

    //ShortestPath Stuff end @@@@@@@@@@@@@@@


    bool isValidPlayer(uint iClientID, bool bCharfile)
    {
        HK_ERROR err;

        if (iClientID < 1 || iClientID > MAX_CLIENT_ID)
            return false;

        if (!HkIsValidClientID(iClientID)) {
            return false;
        }

        if (bCharfile)
        {
        
            if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
                return false;
            std::wstring wscCharFileName;
            if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
                return false;
            }
        
        }
    
        
        return true;
    
    }

    void CharSelectMenu()
    {
        //Check CharSelectMenu
        struct PlayerData* pPD = 0;
        while (pPD = Players.traverse_active(pPD)) {
            uint iClientID = HkGetClientIdFromPD(pPD);


            if (!isValidPlayer(iClientID, false))
                continue;

            if (HkIsInCharSelectMenu(iClientID))
            {
                //PlayerHunt
                if (Modules::GetModuleState("PlayerHunt"))
                {
                    PlayerHunt::CheckDisConnect(iClientID);
                }

                //PVP
                if (Modules::GetModuleState("PVP"))
                {
                    ConPrint (std::to_wstring(iClientID) + L" is in CharSelectMenu\n");
                    PVP::CheckDisConnect(iClientID, PVP::DisconnectReason::CHARSWITCH);
                }

            }



        }

    }

    void FLSRIniDelete(const std::string& scFile, const std::string& scApp,const std::string& scKey) {
        WritePrivateProfileString(scApp.c_str(), scKey.c_str(), NULL,
            scFile.c_str());
    }

}