#pragma once
#include <vector>
#include <unordered_set>
#include "MissionArch.h"

namespace Missions
{
	struct Trigger;

	struct MissionObject
	{
		uint objId;
		uint name;
		std::unordered_set<uint> labels;
		uint clientId = 0;
	};

	struct Mission
	{
		const MissionArchetypePtr archetype;
		bool ended;

		std::vector<MissionObject> objects;
		std::vector<Trigger*> triggers;

		Mission(const MissionArchetypePtr missionArchetype);
		virtual ~Mission();
		void End();
		void RemoveTrigger(const Trigger* trigger);
	};

	bool StartMission(const std::string& missionName);
	bool KillMission(const std::string& missionName);
	void RemoveObjectFromMissions(const uint objId);
}