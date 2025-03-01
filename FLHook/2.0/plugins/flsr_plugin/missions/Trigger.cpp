#include "Trigger.h"
#include "Actions/Action.h"
#include "Conditions/Condition.h"
#include "Conditions/CndTrue.h"
#include "Conditions/CndDestroyed.h"
#include "Actions/ActActTrigger.h"
#include "Actions/ActAddLabel.h"
#include "Actions/ActRemoveLabel.h"
#include "Actions/ActDestroy.h"
#include "Actions/ActChangeState.h"
#include "Actions/ActLightFuse.h"
#include "Actions/ActSpawnSolar.h"
#include <FLHook.h>
#include <queue>

namespace Missions
{
	std::queue<Trigger*> executionQueue;
	bool executionRunning = false;

	static void ExecuteTriggers()
	{
		if (executionRunning || executionQueue.empty())
			return;

		std::unordered_set<Mission*> endedMissions;
		executionRunning = true;
		while (!executionQueue.empty())
		{
			Trigger* trigger = executionQueue.front();
			executionQueue.pop();
			// Skip any further triggers if the mission has been ended.
			if (trigger->mission->ended)
			{
				ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Skip execution due to ended mission.\n");
				continue;
			}
			ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Execute\n");
			for (Action* action : trigger->actions)
				action->Execute();

			if (trigger->mission->ended)
				endedMissions.insert(trigger->mission);

			// Delete the trigger once executed.
			trigger->mission->RemoveTrigger(trigger);
			delete trigger;
		}

		// Clean up all missions which have been ended this run.
		for (Mission* mission : endedMissions)
			delete mission;
		executionRunning = false;
	}

	Trigger::Trigger(Mission* parentMission, const TriggerArchetype& triggerArchetype) :
		name(triggerArchetype.name),
		mission(parentMission)
	{
		condition = NULL;
		switch (triggerArchetype.condition.first)
		{
			case TriggerCondition::Cnd_Destroyed:
				condition = new CndDestroyed(this, (CndDestroyedArchetype*)triggerArchetype.condition.second.get());
				break;

			default:
				condition = new CndTrue(this);
				break;
		}

		for (const auto& actionArchetype : triggerArchetype.actions)
		{
			switch (actionArchetype.first)
			{
				case TriggerAction::Act_ActTrig:
					actions.push_back(new ActActTrigger(this, (ActActTriggerArchetype*)actionArchetype.second.get()));
					break;

				case TriggerAction::Act_DeactTrig:
					actions.push_back(new ActActTrigger(this, (ActActTriggerArchetype*)actionArchetype.second.get()));
					break;

				case TriggerAction::Act_AddLabel:
					actions.push_back(new ActAddLabel(this, (ActAddLabelArchetype*)actionArchetype.second.get()));
					break;

				case TriggerAction::Act_RemoveLabel:
					actions.push_back(new ActRemoveLabel(this, (ActRemoveLabelArchetype*)actionArchetype.second.get()));
					break;

				case TriggerAction::Act_Destroy:
					actions.push_back(new ActDestroy(this, (ActDestroyArchetype*)actionArchetype.second.get()));
					break;

				case TriggerAction::Act_SpawnSolar:
					actions.push_back(new ActSpawnSolar(this, (ActSpawnSolarArchetype*)actionArchetype.second.get()));
					break;

				case TriggerAction::Act_ChangeState:
					actions.push_back(new ActChangeState(this, (ActChangeStateArchetype*)actionArchetype.second.get()));
					break;

				case TriggerAction::Act_LightFuse:
					actions.push_back(new ActLightFuse(this, (ActLightFuseArchetype*)actionArchetype.second.get()));
					break;

				default:
					break;
			}
		}
	}

	Trigger::~Trigger()
	{
		condition->Unregister();
		delete condition;
		for (Action* action : actions)
			delete action;
	}

	void Trigger::Activate()
	{
		ConPrint(stows(mission->name) + L"->" + stows(name) + L": Activate\n");
		if (condition->type == TriggerCondition::Cnd_True)
			QueueExecution();
		else
			condition->Register();
	}

	void Trigger::Deactivate()
	{
		ConPrint(stows(mission->name) + L"->" + stows(name) + L": Deactivate\n");
		condition->Unregister();
	}

	void Trigger::QueueExecution()
	{
		condition->Unregister();
		ConPrint(stows(mission->name) + L"->" + stows(name) + L": Queue execution\n");
		executionQueue.push(this);
		ExecuteTriggers();
	}
}