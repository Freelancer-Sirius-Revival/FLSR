#include <FLHook.h>
#include "../../Plugin.h"
#include "Objectives.h"
#include "Objective.h"
#include "../conditions/Condition.h"

namespace Missions
{
	class ObjCndNpcSimulationRunning : public Condition
	{
	private:
		const ObjectiveParent parent;
		const ObjectiveState state;

		uint targetSystemId;

	public:
		ObjCndNpcSimulationRunning(const ObjectiveParent& parent, const ObjectiveState& state);
		~ObjCndNpcSimulationRunning();
		void Register();
		void Unregister();
		bool Matches(const uint clientId) const;
		void ExecuteTrigger();
	};

	std::unordered_set<ObjCndNpcSimulationRunning*> observedObjCndPlayerInSystemSpace;

	ObjCndNpcSimulationRunning::ObjCndNpcSimulationRunning(const ObjectiveParent& parent, const ObjectiveState& state) :
		Condition(ConditionParent(parent.missionId, 0)),
		parent(parent),
		state(state)
	{
		pub::SpaceObj::GetSystem(state.objId, targetSystemId);
	}

	ObjCndNpcSimulationRunning::~ObjCndNpcSimulationRunning()
	{
		Unregister();
	}

	void ObjCndNpcSimulationRunning::Register()
	{
		struct PlayerData* playerData = nullptr;
		while (playerData = Players.traverse_active(playerData))
		{
			if (Matches(playerData->iOnlineID))
			{
				ExecuteTrigger();
				return;
			}
		}

		observedObjCndPlayerInSystemSpace.insert(this);
	}

	void ObjCndNpcSimulationRunning::Unregister()
	{
		observedObjCndPlayerInSystemSpace.erase(this);
	}

	bool ObjCndNpcSimulationRunning::Matches(const uint clientId) const
	{
		uint systemId;
		pub::Player::GetSystem(clientId, systemId);
		uint baseId;
		pub::Player::GetBase(clientId, baseId);
		return systemId == targetSystemId && !baseId;
	}

	void ObjCndNpcSimulationRunning::ExecuteTrigger()
	{
		Unregister();
		const auto& mission = missions.at(parent.missionId);
		const auto& objectives = mission.objectives.at(parent.objectivesId);
		objectives.objectives[state.objectiveIndex]->Execute(state);
	}

	namespace Hooks
	{
		namespace ObjCndNpcSimulationRunning
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int shipId, unsigned int clientId)
			{
				const std::unordered_set<Missions::ObjCndNpcSimulationRunning*> currentConditions(observedObjCndPlayerInSystemSpace);
				for (const auto& condition : currentConditions)
				{
					if (observedObjCndPlayerInSystemSpace.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall SystemSwitchOutComplete_AFTER(unsigned int shipId, unsigned int clientId)
			{
				const std::unordered_set<Missions::ObjCndNpcSimulationRunning*> currentConditions(observedObjCndPlayerInSystemSpace);
				for (const auto& condition : currentConditions)
				{
					if (observedObjCndPlayerInSystemSpace.contains(condition) && condition->Matches(clientId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}

	Objectives::Objectives(const uint id, const uint missionId) :
		id(id),
		missionId(missionId)
	{}

	void Objectives::Progress(const ObjectiveState& state) const
	{
		// Clean any assigned condition upon progressing
		missions.at(missionId).objectiveConditionByObjectId.erase(state.objId);

		if (state.objectiveIndex >= 0 && state.objectiveIndex < objectives.size())
		{
			const auto& awaitExecutionCnd = ConditionPtr(new ObjCndNpcSimulationRunning(ObjectiveParent(missionId, id), state));
			const auto& insertResult = missions.at(missionId).objectiveConditionByObjectId.insert({ state.objId, awaitExecutionCnd });
			// If there is already a condition for this object, replace it. This will automatically unregister the other condition as by their deconstructor.
			if (!insertResult.second)
				insertResult.first->second = awaitExecutionCnd;
			awaitExecutionCnd->Register();
		}
	}
}