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
		const MissionArchetypePtr archetype;
		bool ended;

		//std::vector<MissionObject> objects;
		std::unordered_map<uint, uint> objectIdsByName;
		std::unordered_map<uint, std::vector<MissionObject>> objectsByLabel;
		std::unordered_set<uint> objectIds;
		std::unordered_set<uint> clientIds;
		std::vector<Trigger*> triggers;

		Mission(const MissionArchetypePtr missionArchetype);
		virtual ~Mission();
		void End();
		void RemoveTrigger(const Trigger* trigger);
		void RemoveObject(const uint objId);
		void RemoveClient(const uint clientId);
	};

	bool StartMission(const std::string& missionName);
	bool KillMission(const std::string& missionName);
	void RemoveObjectFromMissions(const uint objId);
	void RemoveClientFromMissions(const uint client);
}