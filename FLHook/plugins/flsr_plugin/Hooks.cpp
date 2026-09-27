#include "Main.h"

namespace Hooks
{
    //CharacterSelect
    void __stdcall CharacterSelect(struct CHARACTER_ID const &cId, unsigned int iClientID)
    {
        //NewPlayerMessage
        Tools::HkNewPlayerMessage(iClientID, cId);
        returncode = DEFAULT_RETURNCODE;
    }

    // LaunchComplete
    void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip)
    {
        //Get ClientID
        uint iClientID = HkGetClientIDByShip(iShip);
        if (iClientID)
        {
            PopUp::WelcomeBox(iClientID);
            returncode = DEFAULT_RETURNCODE;
            return;
        }
        returncode = DEFAULT_RETURNCODE;
	}
}