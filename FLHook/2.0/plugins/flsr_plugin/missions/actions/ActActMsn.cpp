#include "ActActMsn.h"
#include <random>

namespace Missions
{
	void ActActMsn::Execute(Mission& mission, const MissionObject& activator) const
	{
		for (auto& missionEntry : missions)
		{
			if (CreateID(missionEntry.second.name.c_str()) == nameId)
			{
				auto& foundMission = missionEntry.second;
				if (!foundMission.CanBeStarted())
				{
					ConPrint(L"ERROR: Act_ActMsn could not start already running mission " + std::to_wstring(nameId) + L"\n");
					return;
				}

				const bool transferAllPlayerLabels = playerLabelsToTransfer.size() == 1 && playerLabelsToTransfer.contains(ActActMsnAllPlayerLabels);
				for (const auto& objectLabelsEntry : mission.objectsByLabel)
				{
					if (transferAllPlayerLabels || playerLabelsToTransfer.contains(objectLabelsEntry.first))
					{
						for (const auto& object : objectLabelsEntry.second)
						{
							if (object.type == MissionObjectType::Client)
								foundMission.AddLabelToObject(object, objectLabelsEntry.first);
						}
					}
				}

				foundMission.Start();
				return;
			}
		}
		ConPrint(L"ERROR: Act_ActMsn could not find mission " + std::to_wstring(nameId) + L"\n");
	}
}