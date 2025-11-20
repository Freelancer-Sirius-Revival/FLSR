#include <FLHook.h>
#include "Trigger.h"
#include "actions/Action.h"
#include "conditions/Condition.h"
#include "Conditions/CndTrue.h"

namespace Missions
{
	Trigger::Trigger(const std::string name,
					const uint id,
					const uint missionId,
					const bool initiallyActive,
					const TriggerRepeatable repeatable) :
		name(name),
		id(id),
		nameId(CreateID(name.c_str())),
		missionId(missionId),
		initiallyActive(initiallyActive),
		state(initiallyActive ? TriggerState::AwaitingInitialActivation : TriggerState::Inactive),
		repeatable(repeatable),
		condition(ConditionPtr(new CndTrue({ missionId, id })))
	{}

	Trigger::~Trigger()
	{
		condition->Unregister();
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
		}
	}
}