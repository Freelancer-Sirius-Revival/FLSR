#include <queue>
#include <FLHook.h>
#include "Trigger.h"
#include "Actions/Action.h"
#include "Conditions/Condition.h"
#include "Conditions/CndTrue.h"
#include "Conditions/CndDistVec.h"
#include "Conditions/CndDestroyed.h"
#include "Actions/ActEndMission.h"
#include "Actions/ActActTrigger.h"
#include "Actions/ActAddLabel.h"
#include "Actions/ActRemoveLabel.h"
#include "Actions/ActDestroy.h"
#include "Actions/ActChangeState.h"
#include "Actions/ActLightFuse.h"
#include "Actions/ActSpawnSolar.h"
#include "Actions/ActPlaySoundEffect.h"
#include "Actions/ActPlayMusic.h"
#include "Actions/ActEtherComm.h"
#include "Actions/ActSendComm.h"

namespace Missions
{
	static Condition* instantiateCondition(Trigger* trigger, const TriggerArchConditionEntry& conditionArchetype)
	{
		switch (conditionArchetype.first)
		{
			case TriggerCondition::Cnd_Destroyed:
				return new CndDestroyed(trigger, std::static_pointer_cast<CndDestroyedArchetype>(conditionArchetype.second));

			case TriggerCondition::Cnd_DistVec:
				return new CndDistVec(trigger, std::static_pointer_cast<CndDistVecArchetype>(conditionArchetype.second));

			default:
				return new CndTrue(trigger);
		}
	}

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
				ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Skip execution due to ended mission.\n");
				continue;
			}
			ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Execute\n");
			for (Action* action : trigger->actions)
				action->Execute();

			if (trigger->mission->ended)
				endedMissions.insert(trigger->mission);

			if (!trigger->archetype->repeatable)
			{
				// Delete the trigger once executed.
				trigger->mission->RemoveTrigger(trigger);
				delete trigger;
			}
			else
			{
				// Re-instantiate the condition to reset it.
				for (const auto& triggerArchetype : trigger->mission->archetype->triggers)
				{
					if (triggerArchetype->name == trigger->archetype->name)
					{
						delete trigger->condition;
						trigger->condition = instantiateCondition(trigger, triggerArchetype->condition);
						if (trigger->active)
							trigger->Activate();
						break;
					}
				}
			}
		}

		// Clean up all missions which have been ended this run.
		for (Mission* mission : endedMissions)
			delete mission;
		executionRunning = false;
	}

	Trigger::Trigger(Mission* parentMission, const TriggerArchetypePtr triggerArchetype) :
		archetype(triggerArchetype),
		mission(parentMission),
		active(triggerArchetype->active)
	{
		condition = instantiateCondition(this, archetype->condition);

		for (const auto& actionArchetype : archetype->actions)
		{
			switch (actionArchetype.first)
			{
				case TriggerAction::Act_EndMission:
					actions.push_back(new ActEndMission(this));
					break;

				case TriggerAction::Act_ActTrig:
					actions.push_back(new ActActTrigger(this, std::static_pointer_cast<ActActTriggerArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_DeactTrig:
					actions.push_back(new ActActTrigger(this, std::static_pointer_cast<ActActTriggerArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_AddLabel:
					actions.push_back(new ActAddLabel(this, std::static_pointer_cast<ActAddLabelArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_RemoveLabel:
					actions.push_back(new ActRemoveLabel(this, std::static_pointer_cast<ActRemoveLabelArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_Destroy:
					actions.push_back(new ActDestroy(this, std::static_pointer_cast<ActDestroyArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_SpawnSolar:
					actions.push_back(new ActSpawnSolar(this, std::static_pointer_cast<ActSpawnSolarArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_ChangeState:
					actions.push_back(new ActChangeState(this, std::static_pointer_cast<ActChangeStateArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_LightFuse:
					actions.push_back(new ActLightFuse(this, std::static_pointer_cast<ActLightFuseArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_PlaySoundEffect:
					actions.push_back(new ActPlaySoundEffect(this, std::static_pointer_cast<ActPlaySoundEffectArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_PlayMusic:
					actions.push_back(new ActPlayMusic(this, std::static_pointer_cast<ActPlayMusicArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_EtherComm:
					actions.push_back(new ActEtherComm(this, std::static_pointer_cast<ActEtherCommArchetype>(actionArchetype.second)));
					break;

				case TriggerAction::Act_SendComm:
					actions.push_back(new ActSendComm(this, std::static_pointer_cast<ActSendCommArchetype>(actionArchetype.second)));
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
		active = true;
		ConPrint(stows(mission->archetype->name) + L"->" + stows(archetype->name) + L": Activate\n");
		if (condition->type == TriggerCondition::Cnd_True)
			QueueExecution();
		else
			condition->Register();
	}

	void Trigger::Deactivate()
	{
		active = false;
		ConPrint(stows(mission->archetype->name) + L"->" + stows(archetype->name) + L": Deactivate\n");
		condition->Unregister();
	}

	void Trigger::QueueExecution()
	{
		condition->Unregister();
		ConPrint(stows(mission->archetype->name) + L"->" + stows(archetype->name) + L": Queue execution, activator: ");
		if (condition->activator.type == MissionObjectType::Client)
			ConPrint(L"client[" + std::to_wstring(condition->activator.id) + L"]");
		else
			ConPrint(L"obj[" + std::to_wstring(condition->activator.id) + L"]");
		ConPrint(L"\n");
		executionQueue.push(this);
		ExecuteTriggers();
	}
}