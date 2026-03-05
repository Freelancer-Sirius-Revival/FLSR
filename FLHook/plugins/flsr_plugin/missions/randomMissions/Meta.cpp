#include "Meta.h"
#include <FLHook.h>

#define METACPP_DLLVALUE(TYPE, OFFSET) (*(TYPE*)(DWORD(contentHandle) + (OFFSET)))

namespace RandomMissions
{
	float hostileDockingThreshold = -0.6f;
	float minRequiredReputationForMissions = -0.2f;

	static HMODULE LoadContentDll()
	{
		INI_Reader ini;
		if (!ini.open("freelancer.ini", false))
			return 0;

		while (ini.read_header())
		{
			if (!ini.is_header("Initial MP DLLs")) continue;
			while (ini.read_value())
			{
				if (ini.is_value("path"))
				{
					const std::string dataPath = ini.get_value_string(0);
					ini.close();
					return GetModuleHandle((dataPath + "\\Content.dll").c_str());
				}
			}
			break;
		}
		ini.close();
		return nullptr;
	}

	void ReadMetadata()
	{
		const HMODULE contentHandle = LoadContentDll();
		if (!contentHandle)
			return;

		hostileDockingThreshold = METACPP_DLLVALUE(float, 0x11BCF4);
		minRequiredReputationForMissions = METACPP_DLLVALUE(float, 0x1195BC);
	}
}

#undef METACPP_DLLVALUE
