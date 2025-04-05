#include <queue>
#include <FLHook.h>
#include "Trigger.h"
#include "Mission.h"
#include "Actions/Action.h"
#include "Conditions/Condition.h"
#include "Conditions/CndTrue.h"
#include "Conditions/CndDistVec.h"
#include "Conditions/CndDestroyed.h"
#include "Conditions/CndSpaceEnter.h"
#include "Conditions/CndSpaceExit.h"
#include "Conditions/CndBaseEnter.h"
#include "Conditions/CndTimer.h"
#include "Conditions/CndCount.h"

namespace Missions
{
	static Condition* instantiateCondition(const ConditionParent& parent, const TriggerArchConditionEntry& conditionArchetype)
	{
		Condition* result;
		switch (conditionArchetype.first)
		{
			case ConditionType::Cnd_Destroyed:
				result = new CndDestroyed(parent, std::static_pointer_cast<CndDestroyedArchetype>(conditionArchetype.second));
				break;

			case ConditionType::Cnd_DistVec:
				result = new CndDistVec(parent, std::static_pointer_cast<CndDistVecArchetype>(conditionArchetype.second));
				break;

			case ConditionType::Cnd_SpaceEnter:
				result = new CndSpaceEnter(parent, std::static_pointer_cast<CndSpaceEnterArchetype>(conditionArchetype.second));
				break;

			case ConditionType::Cnd_SpaceExit:
				result = new CndSpaceExit(parent, std::static_pointer_cast<CndSpaceExitArchetype>(conditionArchetype.second));
				break;

			case ConditionType::Cnd_BaseEnter:
				result = new CndBaseEnter(parent, std::static_pointer_cast<CndBaseEnterArchetype>(conditionArchetype.second));
				break;

			case ConditionType::Cnd_Timer:
				result = new CndTimer(parent, std::static_pointer_cast<CndTimerArchetype>(conditionArchetype.second));
				break;

			case ConditionType::Cnd_Count:
				result = new CndCount(parent, std::static_pointer_cast<CndCountArchetype>(conditionArchetype.second));
				break;

			default:
				result = new CndTrue(parent);
				break;
		}
		return result;
	}

	Trigger::Trigger(const unsigned int id, const unsigned int parentMissionId, const TriggerArchetypePtr triggerArchetype) :
		id(id),
		missionId(parentMissionId),
		archetype(triggerArchetype),
		state(TriggerState::Inactive),
		condition(nullptr)
	{}

	Trigger::~Trigger()
	{
		if (condition != nullptr)
			condition->Unregister();
	}

	void Trigger::Activate()
	{
		if (state != TriggerState::Inactive)
			return;
		state = TriggerState::Active;
		if (condition != nullptr)
			condition->Unregister();
		condition = std::shared_ptr<Condition>(instantiateCondition(ConditionParent(missionId, id), archetype->condition));
		condition->Register();
	}

	void Trigger::Deactivate()
	{
		if (state != TriggerState::Active)
			return;
		state = TriggerState::Inactive;
		if (condition != nullptr)
			condition->Unregister();
		condition = nullptr;
	}

	void Trigger::Execute(const MissionObject& activator)
	{
		auto& mission = missions.at(missionId);
		if (condition != nullptr)
			condition->Unregister();
		condition = nullptr;
		for (const auto& action : archetype->actions)
			action->Execute(mission, activator);

		if (archetype->repeatable)
		{
			// In case the trigger wasn't deactivated by itself.
			if (state == TriggerState::Active)
			{
				// Temporarily set inactive to allow re-activation.
				state = TriggerState::Inactive;
				Activate();
			}
		}
		else
		{
			state = TriggerState::Finished;
		}
	}
}