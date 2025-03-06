#pragma once
#include <vector>
#include <unordered_set>
#include "MissionArch.h"
#include "MissionObject.h"

namespace Missions
{
	struct Trigger;

	struct Mission
	{
		const unsigned int id;
		const MissionArchetypePtr archetype;
		bool ended;

		std::unordered_map<uint, uint> objectIdsByName;
		std::unordered_map<uint, std::vector<MissionObject>> objectsByLabel;
		std::unordered_set<uint> objectIds;
		std::unordered_set<uint> clientIds;
		std::unordered_set<unsigned int> triggerIds;

		Mission();
		Mission(const unsigned int id, const MissionArchetypePtr missionArchetype);
		virtual ~Mission();
		void End();
		void RemoveObject(const uint objId);
		void RemoveClient(const uint clientId);
	};
	extern std::unordered_map<unsigned int, Mission> missions;

	bool StartMission(const std::string& missionName);
	bool KillMission(const std::string& missionName);
	void KillMissions();
	void RemoveObjectFromMissions(const uint objId);
	void RemoveClientFromMissions(const uint client);
}