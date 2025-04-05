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
#include "Actions/ActDebugMsg.h"
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
		ConPrint(stows(missions.at(missionId).archetype->name) + L"->" + stows(archetype->name) + L": Activate\n");
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
		ConPrint(stows(missions.at(missionId).archetype->name) + L"->" + stows(archetype->name) + L": Deactivate\n");
		if (condition != nullptr)
			condition->Unregister();
	}

	void Trigger::Execute()
	{
		auto& mission = missions.at(missionId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(archetype->name) + L": Execute through activator ");
		if (activator.type == MissionObjectType::Client)
			ConPrint(L"client[" + std::to_wstring(activator.id) + L"]");
		else
			ConPrint(L"obj[" + std::to_wstring(activator.id) + L"]");
		ConPrint(L"\n");

		if (condition != nullptr)
			condition->Unregister();
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