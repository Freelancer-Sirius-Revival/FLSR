#pragma once
#include <vector>
#include <unordered_set>
#include "MissionArch.h"
#include "MissionObject.h"
#include "Objectives/Objectives.h"

namespace Missions
{
	struct Trigger;

	struct Mission
	{
		const uint id;
		const MissionArchetypePtr archetype;
		bool ended;

		std::unordered_map<uint, uint> objectIdsByName;
		std::unordered_map<uint, std::vector<MissionObject>> objectsByLabel;
		std::unordered_set<uint> objectIds;
		std::unordered_set<uint> clientIds;
		std::unordered_set<uint> triggerIds;
		std::unordered_map<uint, Objectives> objectivesByObjectId;

		Mission();
		Mission(const uint id, const MissionArchetypePtr missionArchetype);
		virtual ~Mission();
		void End();
		void RemoveObject(const uint objId);
		void RemoveClient(const uint clientId);
	};
	extern std::unordered_map<uint, Mission> missions;

	bool StartMission(const std::string& missionName);
	bool KillMission(const std::string& missionName);
	void KillMissions();
	void RemoveObjectFromMissions(const uint objId);
	void RemoveClientFromMissions(const uint client);
}