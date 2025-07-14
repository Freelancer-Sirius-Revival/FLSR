#pragma once
#include "Trigger.h"
#include "MissionObject.h"
#include "MissionOffer.h"
#include "MsnSolar.h"
#include "MsnFormation.h"
#include "Npc.h"
#include "Objectives/Objectives.h"

namespace Missions
{
	class Mission
	{
	private:
		enum class MissionState
		{
			AwaitingInitialActivation,
			Inactive,
			Active,
			Finished
		};

	public:
		const std::string name;
		const uint id;
		const bool initiallyActive;

		MissionOffer offer;
		uint offerId;

		std::vector<Trigger> triggers;
		std::vector<MsnSolar> solars;
		std::unordered_map<uint, Npc> npcs;
		std::unordered_map<uint, MsnNpc> msnNpcs;
		std::unordered_map<uint, MsnFormation> formations;
		std::unordered_map<uint, ObjectivesArchetype> objectives;

		std::unordered_map<uint, uint> objectIdsByName;
		std::unordered_map<uint, std::vector<MissionObject>> objectsByLabel;
		std::unordered_set<uint> objectIds;
		std::unordered_set<uint> clientIds;
		std::unordered_map<uint, Objectives> objectivesByObjectId;

	private:
		MissionState state;
		std::queue<std::pair<uint, MissionObject>> triggerExecutionQueue;

		void EvaluateCountConditions(const uint label) const;

	public:
		Mission(const std::string name, const uint id, const bool initiallyActive);
		virtual ~Mission();
		void Reset();
		bool Start();
		void End();
		void QueueTriggerExecution(const uint triggerId, const MissionObject& activator);
		void AddObject(const uint objId, const uint name, const std::unordered_set<uint> labels);
		void AddLabelToObject(const MissionObject& object, const uint label);
		void RemoveLabelFromObject(const MissionObject& object, const uint label);
		void RemoveObject(const uint objId);
		void RemoveClient(const uint clientId);
	};

	extern std::unordered_map<uint, Mission> missions;
}