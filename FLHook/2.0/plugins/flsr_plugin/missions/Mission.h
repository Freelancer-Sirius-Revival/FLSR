#pragma once
#include <vector>
#include <unordered_set>
#include "MissionArch.h"
#include "Trigger.h"
#include "MissionObject.h"
#include "Objectives/Objectives.h"

namespace Missions
{
	class Mission
	{
	public:
		const uint id;
		const MissionArchetypePtr archetype;
		std::unordered_map<uint, Trigger> triggers;
		std::unordered_map<uint, uint> objectIdsByName;
		std::unordered_map<uint, std::vector<MissionObject>> objectsByLabel;
		std::unordered_set<uint> objectIds;
		std::unordered_set<uint> clientIds;
		std::unordered_map<uint, Objectives> objectivesByObjectId;
	private:
		bool ended = false;
		bool triggerExecutionRunning = false;
		std::queue<std::pair<uint, MissionObject>> triggerExecutionQueue;

	public:
		Mission(const uint id, const MissionArchetypePtr missionArchetype);
		virtual ~Mission();
		void Start();
		void QueueTriggerExecution(const uint triggerId, const MissionObject& activator);
		void End();
		void EvaluateCountConditions(const uint label);
		void AddObject(const uint objId, const uint name, const std::unordered_set<uint> labels);
		void AddLabelToObject(const MissionObject& object, const uint label);
		void RemoveLabelFromObject(const MissionObject& object, const uint label);
		void RemoveObject(const uint objId);
		void RemoveClient(const uint clientId);
	};
	extern std::unordered_map<uint, Mission> missions;
}