#include "Main.h"
#include <unordered_set>

/**
 * SOLAR INVINCIBILITY PLUGIN
 *
 * by Skotty
 * 
 * This plugin allows to mark specific static solars to be invincible toward other NPCs/solars.
 * 
 * Config template:
 * 
 * [Solars]
 * nickname = <solar nickname>
 * 
 */

namespace SolarInvincibility
{
	static std::unordered_set<uint> solarIds;

	void LoadSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-SolarInvincibility.cfg";

		INI_Reader ini;
		if (ini.open(configFilePath.c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Solars"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							solarIds.insert(CreateID(ini.get_value_string(0)));
					}
				}
			}
			ini.close();
		}
	}

    static bool initialized = false;
    void Initialize()
    {
        if (initialized)
            return;
        initialized = true;

        CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
        while (solarIds.size() > 0 && solar != NULL)
        {
            if (solarIds.contains(solar->get_id()))
            {
				solarIds.erase(solar->get_id());
                pub::SpaceObj::SetInvincible(solar->get_id(), true, true, 0);
            }
            solar = static_cast<CSolar*>(solar->FindNext());
        }
		solarIds.clear();
    }
}
