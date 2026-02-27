#include "Main.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <queue>
#include <unordered_map>


namespace Tools {

    std::list <CMPDump_Exception> lCMPUpdateExceptions;

    bool startsWith(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() &&
           0 == str.compare(0, prefix.size(), prefix);
    }

    bool endsWith(std::string_view str, std::string_view suffix) {
        return str.size() >= suffix.size() &&
               0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }

   //Cmpfiles
    void get_cmpExceptions() {
        //Configpath
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + Globals::PLUGIN_CONFIG_FILE;

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
        if (!std::filesystem::exists(std::string(szCurDir) + std::string(Globals::LIBRELANCER_SDK) + "lleditscript.exe"))
        {
            ConPrint(L"Error: Librelancer-SDK is not properly installed please reinstall it to path: \n" + stows(std::string(szCurDir) + std::string(Globals::LIBRELANCER_SDK)) + L"\n");
            return;
        }
   
        //Check if Dumpfolder Exists
        if (!std::filesystem::exists(std::string(szCurDir) + Globals::CMP_DUMP_FOLDER))
        {
            std::filesystem::create_directory(std::string(szCurDir) + Globals::CMP_DUMP_FOLDER);
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
                    std::string pathFileB = std::string(szCurDir) + Globals::CMP_DUMP_FOLDER + f.filename().string();

                    if (!std::filesystem::exists(pathFileB))
                    {
                        ConPrint(L"New CMP found: " + stows(f.filename().string()) + L" - ");
                        std::filesystem::copy_file(p.path(), std::string(szCurDir) + Globals::CMP_DUMP_FOLDER + f.filename().string(), std::filesystem::copy_options::update_existing);
                        std::string parameters = "/C \"\"" + std::string(szCurDir) + std::string(Globals::LIBRELANCER_SDK) + "lleditscript.exe\"" + " \"" + std::string(szCurDir) + std::string(Globals::LIBRELANCER_SDK) + "dumpcmp.cs-script\" \"" + std::string(szCurDir) + Globals::CMP_DUMP_FOLDER + f.filename().string() + "\" > \"" + std::string(szCurDir) + Globals::CMP_DUMP_FOLDER + f.filename().string() + ".cmp_dump\"";
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

                        std::filesystem::copy_file(p.path(), std::string(szCurDir) + Globals::CMP_DUMP_FOLDER + f.filename().string(), std::filesystem::copy_options::update_existing);
                        std::string parameters = "/C \"\"" + std::string(szCurDir) + std::string(Globals::LIBRELANCER_SDK) + "lleditscript.exe\"" + " \"" + std::string(szCurDir) + std::string(Globals::LIBRELANCER_SDK) + "dumpcmp.cs-script\" \"" + std::string(szCurDir) + Globals::CMP_DUMP_FOLDER + f.filename().string() + "\" > \"" + std::string(szCurDir) + Globals::CMP_DUMP_FOLDER + f.filename().string() + ".cmp_dump\"";
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
            std::wstring wscCharFilename = stows((std::string)cId.charFilename);
            char *pAddress = ((char *)hModRemoteClient + ADDR_CLIENT_NEWPLAYER);
            if (!wscCharFilenameBefore.compare(wscCharFilename)) {
                char szNOP[] = {'\x83', '\xC4', '\x08'}; // add esp 08
                WriteProcMem(pAddress, szNOP, 3);

                //ReSpawn

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
}
