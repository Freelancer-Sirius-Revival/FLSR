#include "Main.h"

namespace Solar {
    // Hook from Raikkonen
    int __cdecl HkCreateSolar(uint &iSpaceID, pub::SpaceObj::SolarInfo &solarInfo) {
        // Hack server.dll so it does not call create solar packet send
        char *serverHackAddress = (char *)hModServer + 0x2A62A;
        char serverHack[] = {'\xEB'};
        WriteProcMem(serverHackAddress, &serverHack, 1);

        // Create the Solar
        int returnVal = pub::SpaceObj::CreateSolar(iSpaceID, solarInfo);

        uint dunno;
        IObjInspectImpl *inspect;
        if (GetShipInspect(iSpaceID, inspect, dunno)) {
            CSolar *solar = (CSolar *)inspect->cobject();

            struct SOLAR_STRUCT {
                std::byte dunno[0x100];
            };

            SOLAR_STRUCT packetSolar;

            char *address1 = (char *)hModServer + 0x163F0;
            char *address2 = (char *)hModServer + 0x27950;

            // fill struct
            __asm
            {
                lea ecx, packetSolar
                mov eax, address1
                call eax
                push solar
                lea ecx, packetSolar
                push ecx
                mov eax, address2
                call eax
                add esp, 8
            }

            // Send packet to every client in the system
            struct PlayerData *pPD = 0;
            while (pPD = Players.traverse_active(pPD)) {
                if (pPD->iSystemID == solarInfo.iSystemID)
                    GetClientInterface()->Send_FLPACKET_SERVER_CREATESOLAR(
                        pPD->iOnlineID, (FLPACKET_CREATESOLAR &)packetSolar);
            }
        }

        // Undo the server.dll hack
        char serverUnHack[] = {'\x74'};
        WriteProcMem(serverHackAddress, &serverUnHack, 1);

        return returnVal;
    }

} // namespace Solar