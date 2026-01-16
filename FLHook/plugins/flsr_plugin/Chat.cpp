#include "Main.h"

namespace Chat {
    HK_ERROR HkSendUChat(std::wstring wscCharname, std::wstring wscText) {

        // only for players with matching setting
        struct PlayerData *pPD = 0;
        while (pPD = Players.traverse_active(pPD)) {
            uint iClientID = HkGetClientIdFromPD(pPD);
            uchar cFormat;
            // adjust chatsize
            switch (ClientInfo[iClientID].chatSize) {
            case CS_SMALL:
                cFormat = 0x90;
                break;

            case CS_BIG:
                cFormat = 0x10;
                break;

            default:
                cFormat = 0x00;
                break;
            }

            // adjust chatstyle
            switch (ClientInfo[iClientID].chatStyle) {
            case CST_BOLD:
                cFormat += 0x01;
                break;

            case CST_ITALIC:
                cFormat += 0x02;
                break;

            case CST_UNDERLINE:
                cFormat += 0x04;
                break;

            default:
                cFormat += 0x00;
                break;
            }

            wchar_t wszFormatBuf[8];
            swprintf(wszFormatBuf, L"%02X", (long)cFormat);
            std::wstring wscTRADataFormat = wszFormatBuf;
            std::wstring wscTRADataColor = L"80E0FF";
            std::wstring wscTRADataSenderColor = L"FFFFFF";

            std::wstring wscXML =
                L"<TRA data=\"0x" + wscTRADataSenderColor + wscTRADataFormat +
                L"\" mask=\"-1\"/><TEXT>" + XMLText(wscCharname) + L": </TEXT>" +
                L"<TRA data=\"0x" + wscTRADataColor + wscTRADataFormat +
                L"\" mask=\"-1\"/><TEXT>" + XMLText(wscText) + L"</TEXT>";
            HkFMsg(iClientID, wscXML);
        }

        return HKE_OK;
    }
} // namespace Chat
