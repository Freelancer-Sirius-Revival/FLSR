#include "ActActMsnTrig.h"
#include <random>

namespace Missions
{
	void ActActMsnTrig::Execute(Mission& mission, const MissionObject& activator) const
	{
		for (auto& missionEntry : missions)
		{
			if (CreateID(missionEntry.second.name.c_str()) == missionId)
			{
				auto& foundMission = missionEntry.second;
				if (foundMission.IsActive())
					ActActTrig::Execute(foundMission, activator);
				else
					ConPrint(L"ERROR: Act_ActMsnTrig could not activate trigger of not active mission " + std::to_wstring(missionId) + L"\n");
				return;
			}
		}
		ConPrint(L"ERROR: Act_ActMsnTrig could not find mission " + std::to_wstring(missionId) + L"\n");
	}
}