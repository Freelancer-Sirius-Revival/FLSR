#pragma once
#include <vector>
#include <unordered_set>
#include "MissionArch.h"

namespace Missions
{
	struct Trigger;

	struct MissionObject
	{
		unsigned int id;
		std::string name;
		std::unordered_set<std::string> labels;
		uint clientId = 0;
	};

	struct Mission
	{
		const MissionArchetype& archetype;
		bool ended;
		const std::string name;
		const int reward;
		const unsigned int titleId;
		const unsigned int offerId;

		std::vector<MissionObject> objects;
		std::vector<Trigger*> triggers;

		Mission(const MissionArchetype& missionArchetype);
		virtual ~Mission();
		void End();
		void RemoveTrigger(const Trigger* trigger);
	};

	bool StartMission(const std::string& missionName);
	bool KillMission(const std::string& missionName);
	void RemoveObjectFromMissions(const uint objId);
}