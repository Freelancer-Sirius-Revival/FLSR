#include <FLHook.h>
#include "Trigger.h"
#include "actions/Action.h"
#include "actions/ActAddLabel.h"
#include "actions/ActRemoveLabel.h"
#include "conditions/Condition.h"
#include "conditions/CndTrue.h"

namespace Missions
{
	Trigger::Trigger(const std::string& name,
					const uint id,
					const uint missionId,
					const bool initiallyActive,
					const TriggerRepeatable repeatable) :
		name(name),
		id(id),
		missionId(missionId),
		initiallyActive(initiallyActive),
		state(initiallyActive ? TriggerState::AwaitingInitialActivation : TriggerState::Inactive),
		repeatable(repeatable),
		condition(ConditionPtr(new CndTrue({ missionId, id }))),
		originId(0)
	{
		sortPosition = missions.at(missionId).triggers.size();
	}

	Trigger::~Trigger()
	{
		condition->Unregister();

		// Unregister from origin trigger and clean up player labels.
		if (originId != 0)
		{
			auto& mission = missions.at(missionId);
			ActRemoveLabel action;
			action.objNameOrLabel = id;
			action.label = id;
			action.Execute(mission, MissionObject(MissionObjectType::Client, 0));
			mission.triggers.at(originId).branchIds.erase(id);
		}
	}

	uint Trigger::CreateBranch(const MissionObject activator)
	{
		auto& mission = missions.at(missionId);
		const std::string newTriggerName = name + " for " + (activator.type == MissionObjectType::Object ? "object" : "client") + ":" + std::to_string(activator.id);
		const uint triggerId = CreateID(newTriggerName.c_str());

		// Do not allow multiple branches for the very same activator.
		if (mission.triggers.contains(triggerId))
			return 0;

		ActAddLabel addLabel;
		addLabel.objNameOrLabel = Activator;
		addLabel.label = CreateID(newTriggerName.c_str());
		addLabel.Execute(mission, activator);

		mission.triggers.try_emplace(triggerId, newTriggerName, triggerId, missionId, false, TriggerRepeatable::Off);

		// Register to the origin trigger, or this one.
		if (originId != 0)
			mission.triggers.at(originId).branchIds.insert(triggerId);
		else
			branchIds.insert(triggerId);

		Trigger& clonedTrigger = mission.triggers.at(triggerId);
		clonedTrigger.originId = originId != 0 ? originId : id;
		const ConditionParent cloneParent({ clonedTrigger.missionId, clonedTrigger.id });
		clonedTrigger.condition = condition != nullptr ? condition->Copy(cloneParent, addLabel.label) : ConditionPtr(new CndTrue(cloneParent, addLabel.label));
		clonedTrigger.actions = actions;
		clonedTrigger.Activate();

		return clonedTrigger.id;
	}

	bool Trigger::IsAwaitingInitialActivation() const
	{
		return state == TriggerState::AwaitingInitialActivation;
	}

	void Trigger::Reset()
	{
		condition->Unregister();
		state = initiallyActive ? TriggerState::AwaitingInitialActivation : TriggerState::Inactive;
	}

	void Trigger::Activate()
	{
		if (state != TriggerState::Inactive && state != TriggerState::AwaitingInitialActivation)
			return;
		state = TriggerState::Active;
		condition->Register(); // Some conditions can instantly finish and queue trigger execution.
	}

	void Trigger::Deactivate()
	{
		if (state != TriggerState::Active)
			return;
		state = TriggerState::Inactive;
		condition->Unregister();

		// Clean up all branched triggers.
		if (!branchIds.empty())
		{
			auto& mission = missions.at(missionId);
			const std::unordered_set currentBranchIds(branchIds);
			for (const auto& branchId : currentBranchIds)
				mission.triggers.erase(branchId);
		}
	}

	void Trigger::Execute(const MissionObject& activator)
	{
		condition->Unregister();
		// In case the trigger was deactivated again in between.
		if (state != TriggerState::Active)
			return;

		auto& mission = missions.at(missionId);
		for (const auto& action : actions)
			action->Execute(mission, activator);

		if (repeatable != TriggerRepeatable::Off)
		{
			// In case the trigger wasn't already deactivated by itself.
			if (state == TriggerState::Active)
			{
				state = TriggerState::Inactive;
				if (repeatable == TriggerRepeatable::Auto)
					Activate();
			}
		}
		else
		{
			state = TriggerState::Finished;
			// Triggers that are branched must clean themselves up here.
			if (originId != 0)
				mission.triggers.erase(id);
		}
	}
}