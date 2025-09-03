#pragma once
#include "Trigger.h"
#include "MissionObject.h"
#include "MissionOffer.h"
#include "MsnSolar.h"
#include "MsnFormation.h"
#include "Npc.h"
#include "Dialog.h"
#include <queue>
#include "Objectives/Objectives.h"

namespace Missions
{
	class Mission
	{
	public:
		enum class MissionState
		{
			AwaitingInitialActivation,
			Inactive,
			Active,
			Finished
		};
		enum class MissionResult
		{
			Success,
			Failure
		};
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
		std::unordered_map<uint, Objectives> objectives;
		std::unordered_map<uint, Dialog> dialogs;

		std::unordered_map<uint, uint> objectIdsByName;
		std::unordered_map<uint, std::vector<MissionObject>> objectsByLabel;
		std::unordered_set<uint> objectIds;
		std::unordered_set<uint> clientIds;
		std::unordered_map<Condition*, ConditionPtr> dynamicConditions;
		std::unordered_map<uint, ConditionPtr> objectiveConditionByObjectId;
		std::unordered_map<uint, std::unordered_set<uint>> markedObjIdsByClientId;

		MissionResult missionResult;
		float reofferRemainingTime;

		struct CommEntry
		{
			mstime sendTime;
			uint voiceLineId;
			MissionObject sender = { MissionObjectType::Client, 0 };
			std::unordered_set<uint> receiverObjIds;
		};
		std::unordered_map<uint, CommEntry> ongoingComms;

	private:
		MissionState state;
		std::queue<std::pair<uint, MissionObject>> triggerExecutionQueue;

		void EvaluateCountConditions(const uint label) const;

	public:
		Mission(const std::string name, const uint id, const bool initiallyActive);
		virtual ~Mission();
		bool CanBeStarted() const;
		bool IsActive() const;
		void Start();
		void End();
		void QueueTriggerExecution(const uint triggerId, const MissionObject& activator);
		void AddObject(const uint objId, const uint name, const std::unordered_set<uint> labels);
		void AddLabelToObject(const MissionObject& object, const uint label);
		void RemoveLabelFromObject(const MissionObject& object, const uint label);
		void RemoveObject(const uint objId);
		void RemoveClient(const uint clientId);
		uint FindObjNameByObjId(const uint objId) const;
	};

	extern std::unordered_map<uint, Mission> missions;

	namespace Hooks
	{
		namespace Mission
		{
			void __stdcall Elapse_Time_AFTER(float seconds);
			int __cdecl Dock_Call(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response);
		}
	}
}