#include "Meta.h"

#define METACPP_DLLVALUE(TYPE, OFFSET) (*(TYPE*)(DWORD(contentHandle) + (OFFSET)))

namespace RandomMissions
{
	std::vector<float> npcWaveSizeLikelihoods;
	float npcSpawnDistance;
	float solarSpawnDistance;
	float secondWaypointDistance;
	float npcStayInRangeDistance;
	float playerStayInRangeDistance;
	float playerStayInRangeDistanceTolerance;
	float returnToMissionTimeSeconds;

	static HMODULE LoadContentDll()
	{
		INI_Reader ini;
		if (!ini.open("freelancer.ini", false)) return 0;

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

	void ReadMetadata() {
		const HMODULE contentHandle = LoadContentDll();
		if (!contentHandle) return;

		for (uint i = 0; i <= 8; i++)
		{
			size_t offset = 0x11CC7C + i * sizeof(float);
			npcWaveSizeLikelihoods.push_back(METACPP_DLLVALUE(float, offset));
		}
		npcSpawnDistance = METACPP_DLLVALUE(float, 0x0F17D9);
		solarSpawnDistance = METACPP_DLLVALUE(float, 0x11C2B0);
		secondWaypointDistance = METACPP_DLLVALUE(float, 0x11CBCC);
		npcStayInRangeDistance = METACPP_DLLVALUE(float, 0x11C2CC);
		playerStayInRangeDistance = METACPP_DLLVALUE(float, 0x11C2C8);
		playerStayInRangeDistanceTolerance = METACPP_DLLVALUE(float, 0x11CAB4);
		returnToMissionTimeSeconds = METACPP_DLLVALUE(float, 0x11C2DC);
	}
}

#undef METACPP_DLLVALUE
