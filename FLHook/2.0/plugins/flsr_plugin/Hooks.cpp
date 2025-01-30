#include "Main.h"

namespace Hooks {


    //PopUpDialog
    void __stdcall PopUpDialog(unsigned int iClientID, unsigned int buttonClicked) {
        returncode = DEFAULT_RETURNCODE;

        PopUp::HandleButtonClick(iClientID, buttonClicked);
    }

    //CharacterSelect
    void __stdcall CharacterSelect(struct CHARACTER_ID const &cId, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;

        //NewPlayerMessage
        Tools::HkNewPlayerMessage(iClientID, cId);
    }

    // LaunchComplete
    void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip) {
        returncode = DEFAULT_RETURNCODE;

        //Get ClientID
        uint iClientID = HkGetClientIDByShip(iShip);
        if (!iClientID)
            return;

        //Show WelcomePopUp
        if (Modules::GetModuleState("WelcomeMSG"))
        {
            PopUp::WelcomeBox(iClientID);
        }
		    }

    void SendDeathMsg(const std::wstring& wscMsg, uint iSystemID, uint iClientIDVictim, uint iClientIDKiller) {
        returncode = DEFAULT_RETURNCODE;

        if (!HkIsValidClientID(iClientIDVictim))
            return;

        std::wstring victim, killer;
		Tools::eDeathTypes DeathType;
        // Extract victim and type from the death message
        if (wscMsg.find(L"was killed by an NPC") != std::wstring::npos) {
                    size_t victimStart = wscMsg.find(L"Death: ") + 7;
                    size_t victimEnd = wscMsg.find(L" was killed by an NPC");
                    victim = wscMsg.substr(victimStart, victimEnd - victimStart);
                    DeathType = Tools::PVE;

                    iClientIDKiller = 0;

        }
        else if (wscMsg.find(L"was killed by an admin") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" was killed by an admin");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);
            DeathType = Tools::ADMIN;
            iClientIDKiller = 0;

        }
        else if (wscMsg.find(L" was killed by ") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" was killed by ");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);

            size_t killerStart = victimEnd + 15; // length of " was killed by "
            size_t killerEnd = wscMsg.find(L" (", killerStart);
            killer = wscMsg.substr(killerStart, killerEnd - killerStart);

            size_t typeStart = killerEnd + 2; // skip the " ("
            size_t typeEnd = wscMsg.find(L")", typeStart);
            DeathType = Tools::PVP;
        }
        else if (wscMsg.find(L"killed himself") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" killed himself");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);

            size_t typeStart = wscMsg.find_last_of(L"(") + 1;
            size_t typeEnd = wscMsg.find_last_of(L")");
            DeathType = Tools::KILLEDHIMSELF;

            iClientIDKiller = 0;
        }
        else if (wscMsg.find(L"committed suicide") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" committed suicide");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);
            DeathType = Tools::SUICIDE;
            iClientIDKiller = 0;
        }
        else if (wscMsg.find(L" has died") != std::wstring::npos) {
            size_t victimStart = wscMsg.find(L"Death: ") + 7;
            size_t victimEnd = wscMsg.find(L" has died");
            victim = wscMsg.substr(victimStart, victimEnd - victimStart);
            DeathType = Tools::HASDIED;
            iClientIDKiller = 0;
        }


        // Remove whitespaces from victim
        victim.erase(std::remove_if(victim.begin(), victim.end(), [](unsigned char c) { return std::isspace(c); }), victim.end());
    
        // Remove whitespaces from killer
        killer.erase(std::remove_if(killer.begin(), killer.end(), [](unsigned char c) { return std::isspace(c); }), killer.end());    
    }


    //BaseEnter_AFTER
    void __stdcall BaseEnter_AFTER(unsigned int iBaseID,unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;

        if (Modules::GetModuleState("InsuranceModule"))
        {
            Insurance::UseInsurance(iClientID);
        }
		
    }

    void __stdcall PlayerLaunch_After(unsigned int iShip, unsigned int iClientID) {
        returncode = DEFAULT_RETURNCODE;

        //Insurance
        if (Modules::GetModuleState("InsuranceModule") && !Insurance::IsInsurancePresent(iClientID))
        {
            const bool insuranceRequested = Insurance::IsInsuranceRequested(iClientID);
            Insurance::CreateNewInsurance(iClientID, !insuranceRequested);
        }
    }
}