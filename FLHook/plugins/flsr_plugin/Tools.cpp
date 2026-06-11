#include "Main.h"

#define ADDR_CLIENT_NEWPLAYER 0x8010

namespace Tools {
    std::wstring CS_wscCharBefore;
    void HkNewPlayerMessage(uint iClientID, struct CHARACTER_ID const &cId) {

        //Valid ID
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
