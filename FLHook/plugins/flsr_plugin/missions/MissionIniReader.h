#pragma once
#include <FLHook.h>
#include "Mission.h"

namespace Missions
{
	bool TryReadMissionHeadFromIni(const uint nextMissionId, INI_Reader& ini);
	bool TryReadMsnFormationFromIni(Mission& mission, INI_Reader& ini);
	bool TryReadMsnSolarFromIni(Mission& mission, INI_Reader& ini);
	bool TryReadNpcFromIni(Mission& mission, INI_Reader& ini);
	bool TryReadMsnNpcFromIni(Mission& mission, INI_Reader& ini);
	bool TryReadObjectiveListFromIni(Mission& mission, INI_Reader& ini);
	bool TryReadDialogFromIni(Mission& mission, INI_Reader& ini);
}