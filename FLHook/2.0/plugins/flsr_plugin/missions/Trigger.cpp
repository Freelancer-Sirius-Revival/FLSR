#include <queue>
#include <FLHook.h>
#include "Trigger.h"
#include "Actions/Action.h"
#include "Conditions/Condition.h"
#include "Conditions/CndTrue.h"
#include "Conditions/CndDistVec.h"
#include "Conditions/CndDestroyed.h"
#include "Conditions/CndSpaceEnter.h"
#include "Conditions/CndSpaceExit.h"
#include "Conditions/CndBaseEnter.h"
#include "Conditions/CndTimer.h"
#include "Actions/ActEndMission.h"
#include "Actions/ActActTrigger.h"
#include "Actions/ActAddLabel.h"
#include "Actions/ActRemoveLabel.h"
#include "Actions/ActDestroy.h"
#include "Actions/ActLightFuse.h"
#include "Actions/ActSpawnSolar.h"
#include "Actions/ActSpawnShip.h"
#include "Actions/ActPlaySoundEffect.h"
#include "Actions/ActPlayMusic.h"
#include "Actions/ActEtherComm.h"
#include "Actions/ActSendComm.h"
#include "Actions/ActSetNNObj.h"
#include "Actions/ActAdjAcct.h"
#include "Actions/ActAddCargo.h"
#include "Actions/ActGiveObjList.h"

namespace Missions
{
	static ConditionPtr instantiateCondition(const ConditionParent& parent, const TriggerArchConditionEntry& conditionArchetype)
	{
		Condition* result;
		switch (conditionArchetype.first)
		{
			case TriggerCondition::Cnd_Destroyed:
				result = new CndDestroyed(parent, std::static_pointer_cast<CndDestroyedArchetype>(conditionArchetype.second));
				break;

			case TriggerCondition::Cnd_DistVec:
				result = new CndDistVec(parent, std::static_pointer_cast<CndDistVecArchetype>(conditionArchetype.second));
				break;

			case TriggerCondition::Cnd_SpaceEnter:
				result = new CndSpaceEnter(parent, std::static_pointer_cast<CndSpaceEnterArchetype>(conditionArchetype.second));
				break;

			case TriggerCondition::Cnd_SpaceExit:
				result = new CndSpaceExit(parent, std::static_pointer_cast<CndSpaceExitArchetype>(conditionArchetype.second));
				break;

			case TriggerCondition::Cnd_BaseEnter:
				result = new CndBaseEnter(parent, std::static_pointer_cast<CndBaseEnterArchetype>(conditionArchetype.second));
				break;

			case TriggerCondition::Cnd_Timer:
				result = new CndTimer(parent, std::static_pointer_cast<CndTimerArchetype>(conditionArchetype.second));
				break;

			default:
				result = new CndTrue(parent);
				break;
		}
		return ConditionPtr(result);
	}

	std::queue<unsigned int> executionQueue;
	bool executionRunning = false;

	static void ExecuteTriggers()
	{
		if (executionRunning || executionQueue.empty())
			return;

		std::unordered_set<unsigned int> endedMissionIds;
		executionRunning = true;
		while (!executionQueue.empty())
		{
			const Trigger& trigger = triggers[executionQueue.front()];
			executionQueue.pop();
			// Skip any further triggers if the mission has been ended.
			if (missions[trigger.parentMissionId].ended)
			{
				ConPrint(stows(missions[trigger.parentMissionId].archetype->name) + L"->" + stows(trigger.archetype->name) + L": Skip execution due to ended mission.\n");
				continue;
			}
			ConPrint(stows(missions[trigger.parentMissionId].archetype->name) + L"->" + stows(trigger.archetype->name) + L": Execute\n");
			for (const auto& action : trigger.actions)
				action->Execute();

			if (missions[trigger.parentMissionId].ended)
				endedMissionIds.insert(trigger.parentMissionId);

			if (!trigger.archetype->repeatable)
			{
				// Delete the trigger once executed.
				missions[trigger.parentMissionId].triggerIds.erase(trigger.id);
				triggers.erase(trigger.id);
			}
			else
			{
				// Re-instantiate the condition to reset it.
				for (const auto& triggerArchetype : missions[trigger.parentMissionId].archetype->triggers)
				{
					if (triggerArchetype->name == trigger.archetype->name)
					{
						ConditionParent cndParent(trigger.parentMissionId, trigger.id);
						(ConditionPtr)trigger.condition = instantiateCondition(cndParent, triggerArchetype->condition);
						if (trigger.active)
							trigger.condition->Register();
						break;
					}
				}
			}
		}

		// Clean up all missions which have been ended this run.
		for (const auto& id : endedMissionIds)
			missions.erase(id);

		executionRunning = false;
	}

	std::unordered_map<unsigned int, Trigger> triggers;

	Trigger::Trigger() :
		id(0),
		parentMissionId(0),
		archetype(nullptr),
		active(false),
		condition(nullptr)
	{}

	Trigger::Trigger(const unsigned int id, const unsigned int parentMissionId, const TriggerArchetypePtr triggerArchetype) :
		id(id),
		parentMissionId(parentMissionId),
		archetype(triggerArchetype),
		active(false)
	{
		ConditionParent cndParent(parentMissionId, id);
		condition = instantiateCondition(cndParent, archetype->condition);

		ActionParent actParent;
		actParent.missionId = parentMissionId;
		actParent.triggerId = id;

		for (const auto& actionArchetype : archetype->actions)
		{
			Action* result = nullptr;
			switch (actionArchetype.first)
			{
				case TriggerAction::Act_EndMission:
					result = new ActEndMission(actParent);
					break;

				case TriggerAction::Act_ActTrig:
					result = new ActActTrigger(actParent, std::static_pointer_cast<ActActTriggerArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_DeactTrig:
					result = new ActActTrigger(actParent, std::static_pointer_cast<ActActTriggerArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_AddLabel:
					result = new ActAddLabel(actParent, std::static_pointer_cast<ActAddLabelArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_RemoveLabel:
					result = new ActRemoveLabel(actParent, std::static_pointer_cast<ActRemoveLabelArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_Destroy:
					result = new ActDestroy(actParent, std::static_pointer_cast<ActDestroyArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_SpawnSolar:
					result = new ActSpawnSolar(actParent, std::static_pointer_cast<ActSpawnSolarArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_SpawnShip:
					result = new ActSpawnShip(actParent, std::static_pointer_cast<ActSpawnShipArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_LightFuse:
					result = new ActLightFuse(actParent, std::static_pointer_cast<ActLightFuseArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_PlaySoundEffect:
					result = new ActPlaySoundEffect(actParent, std::static_pointer_cast<ActPlaySoundEffectArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_PlayMusic:
					result = new ActPlayMusic(actParent, std::static_pointer_cast<ActPlayMusicArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_EtherComm:
					result = new ActEtherComm(actParent, std::static_pointer_cast<ActEtherCommArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_SendComm:
					result = new ActSendComm(actParent, std::static_pointer_cast<ActSendCommArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_SetNNObj:
					result = new ActSetNNObj(actParent, std::static_pointer_cast<ActSetNNObjArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_AdjAcct:
					result = new ActAdjAcct(actParent, std::static_pointer_cast<ActAdjAcctArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_AddCargo:
					result = new ActAddCargo(actParent, std::static_pointer_cast<ActAddCargoArchetype>(actionArchetype.second));
					break;

				case TriggerAction::Act_GiveObjList:
					result = new ActGiveObjList(actParent, std::static_pointer_cast<ActGiveObjListArchetype>(actionArchetype.second));
					break;

				default:
					break;
			}
			if (result != nullptr)
				actions.push_back(ActionPtr(result));
		}
	}

	Trigger::~Trigger()
	{
		condition->Unregister();
	}

	void Trigger::Activate()
	{
		if (active)
			return;
		active = true;
		ConPrint(stows(missions[parentMissionId].archetype->name) + L"->" + stows(archetype->name) + L": Activate\n");
		condition->Register();
	}

	void Trigger::Deactivate()
	{
		if (!active)
			return;
		active = false;
		ConPrint(stows(missions[parentMissionId].archetype->name) + L"->" + stows(archetype->name) + L": Deactivate\n");
		condition->Unregister();
	}

	void Trigger::QueueExecution()
	{
		condition->Unregister();
		ConPrint(stows(missions[parentMissionId].archetype->name) + L"->" + stows(archetype->name) + L": Queue execution, activator: ");
		if (condition->activator.type == MissionObjectType::Client)
			ConPrint(L"client[" + std::to_wstring(condition->activator.id) + L"]");
		else
			ConPrint(L"obj[" + std::to_wstring(condition->activator.id) + L"]");
		ConPrint(L"\n");
		executionQueue.push(id);
		ExecuteTriggers();
	}
}