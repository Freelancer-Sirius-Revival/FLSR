#include "main.h"

namespace SendCash {

    void LogTransfer(std::wstring wscToCharname, std::wstring msg) {
        std::string logFile = GetUserFilePath(wscToCharname, Globals::SENDCASHLOG_FILE);
        if (logFile.empty())
            return;
        FILE *f;
        fopen_s(&f, logFile.c_str(), "at");
        if (!f)
            return;

        try {
            for (uint i = 0; (i < msg.length()); i++) {
                char cHiByte = msg[i] >> 8;
                char cLoByte = msg[i] & 0xFF;
                fprintf(f, "%02X%02X", ((uint)cHiByte) & 0xFF,
                        ((uint)cLoByte) & 0xFF);
            }
            fprintf(f, "\n");
        } catch (...) {
        }
        fclose(f);
    }

} // namespace SendCash